# Option Pricing in C++

A small C++20 library for pricing options, with the reasoning behind its
design and the results it produces:

- **European options** by the generalised Black-Scholes formula (stock,
  futures and other cost-of-carry models), with exact greeks and put-call
  parity used both as a calculator and as a check
- **Perpetual American options** in closed form
- **Numerical greeks** by divided differences, for any option class
- **Monte Carlo** pricing with Euler-Maruyama and exact GBM stepping
- **Finite differences**: an explicit scheme for the Black-Scholes PDE,
  with a built-in stability check
- **Matrix pricing**: one generic mechanism that applies any computation
  (prices, greeks, parity checks) to a whole range of parameters

All 93 automated checks in `Main.cpp` pass.

---

## 1. Building and running

The repository builds two programs from the same classes:

| Program | Source | What it does |
|---|---|---|
| `OptionPricing` | `Main.cpp` | Tests every public function against reference values; exits non-zero if any check fails |
| `CompareMethods` | `CompareMethods.cpp` | Prices the same options by closed form, Monte Carlo and finite differences |

You need a C++20 compiler and the Boost headers. Only Boost.Math is used,
and it is header-only, so there is nothing to build or link.

**Visual Studio.** Open `OptionPricing.sln`, which contains both projects.
Right-click the one you want and choose *Set as Startup Project*. If Boost
is not already on your include path (for example through
`vcpkg integrate install`), set an environment variable `BOOST_ROOT` to the
folder that contains `boost/` and restart Visual Studio. The projects use
the v145 toolset (Visual Studio 2026); older versions offer to retarget
them when the solution is opened.

**CMake** (Linux, macOS or Windows):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/OptionPricing
./build/CompareMethods
ctest --test-dir build
```

Add `-DBOOST_ROOT=/path/to/boost` if Boost is not found. With a
multi-configuration generator such as Visual Studio, build with
`cmake --build build --config Release` and look for the programs under
`build/Release/`.

With GCC 13 and `-Wall -Wextra -Wnon-virtual-dtor -pedantic`, both programs
compile without warnings.

---

## 2. Design

### 2.1 Overview

| File | Responsibility |
|---|---|
| `OptionData.hpp` | The six parameters of an option, plus call/put |
| `OptionException.hpp` | Exception hierarchy for invalid input |
| `Option.hpp/.cpp` | Abstract base: data, accessors, validation, `Price()` |
| `EuropeanOption.hpp/.cpp` | Black-Scholes price, greeks, put-call parity |
| `PerpetualAmericanOption.hpp/.cpp` | Perpetual American price |
| `MeshGenerator.hpp/.cpp` | Mesh arrays and parameter matrices |
| `Pricer.hpp` | Applying any computation to a matrix |
| `DividedDifferences.hpp/.cpp` | Numerical greeks |
| `MonteCarloPricer.hpp/.cpp` | Path simulation, Euler and exact stepping |
| `FiniteDifferencePricer.hpp/.cpp` | Explicit finite differences for the PDE |
| `Main.cpp` | Tests every public function against expected values |
| `CompareMethods.cpp` | Closed form, Monte Carlo and finite differences side by side |

```
                 Option  (abstract)
                 /                \
      EuropeanOption        PerpetualAmericanOption

    MeshGenerator  ->  vector<OptionData>  ->  Pricer  ->  vector<double>

    DividedDifferences  works on  const Option&  (either class)

    MonteCarloPricer, FiniteDifferencePricer  work on  OptionData
```

### 2.2 Why a class hierarchy rather than one class

The two options share their parameters and both must produce a price, but
the price is arrived at in completely different ways: one integrates over
a finite life, the other has no expiry at all and solves a
time-independent problem. A single class with a flag would mean a branch
in every function and an object carrying a `T` that is meaningless half
the time.

Making `Price()` pure virtual means the matrix pricer and the divided
difference class can be written once against `Option` and work for both —
and for anything added later. The output shows this working: the same
`Pricer::Price<T>` serves both classes, and `DividedDifferences` computes
a delta for a perpetual option without having been told what it was given.

### 2.3 Why `OptionData` is a struct, not a class

Six independent numbers with no invariant binding them and no behaviour
of their own. There is nothing for encapsulation to protect. Grouping
them keeps the pricing interfaces short, and a *matrix of option
parameters* becomes `std::vector<OptionData>` — which is what makes the
matrix pricer natural rather than contrived.

Validation lives in `Option`, not here, because what counts as valid
depends on the formula: a European option needs `T > 0`, a perpetual one
does not care about `T` but needs `r > 0`.

### 2.4 Why `enum class OptionType` rather than a string or a bool

`optType == "C"` compiles with any string. A typo becomes a put. An
`enum class` makes the mistake impossible and reads better at the call
site: `OptionType::Call` says what `true` cannot.

### 2.5 Why `N(x)` comes from Boost

A common shortcut is a three-term polynomial approximation to the normal
distribution, accurate to about 1e-5. Boost's
`cdf(normal_distribution<>(0,1), x)` is accurate to machine precision, and
it is one line. Boost supplies the density `n(x)` the same way.

This matters for the divided-difference work: with an `N(x)` good to
1e-5, the noise floor in the numerator would swamp the approximation
error long before h got small, and the error table in section 4 would not
show second-order convergence at all.

### 2.6 Why the matrix pricer takes a function

The flexibility is in *not fixing what is computed*:

```cpp
template <typename F>
std::vector<double> Compute(const std::vector<OptionData>& matrix, F f);
```

One function serves prices, deltas, gammas, vegas and perpetual American
values. The alternative — `PriceMatrix`, `DeltaMatrix`, `GammaMatrix` —
is the same loop written three times, with a fourth needed for every
quantity added later. A second overload, `ComputeAs<R>`, returns
something other than a double; the test program uses it to produce a
vector of parity checks.

This single mechanism produces the matrices in sections 3.4, 4.3 and 5.3.

### 2.7 Why `Option` has a virtual `Clone()`

`DividedDifferences` holds a `const Option&` and must reprice at `S ± h`.
It cannot construct another option: it does not know which class it is
holding. The object does, so it is asked to copy itself — the Prototype
pattern.

The alternative, mutating the caller's option and putting it back, would
need a non-const reference, would leave the object disturbed if a price
threw, and would make the function unusable on a const option.

`Clone()` returns `std::unique_ptr<Option>`, so ownership is explicit and
the copy is released even if the code that follows throws.

The result is that `DividedDifferences` is completely decoupled from the
option hierarchy. It already works on perpetual American options — which
have no exact greeks to compare against, and so are exactly the case the
technique exists for.

### 2.8 Why setters validate and roll back

```cpp
void Option::Data(const OptionData& newData)
{
    OptionData old = m_data;
    m_data = newData;
    try { Validate(); }
    catch (const OptionException&) { m_data = old; throw; }
}
```

An object can never hold a parameter set its formulae cannot handle, and
a rejected set leaves it exactly as it was. The test program checks this:
after `opt.Sigma(-1.0)` throws, the price is unchanged.

### 2.9 Why parity comparison uses a tolerance

`CheckParity` compares two floating point numbers arrived at by different
routes. They will rarely be bit-for-bit equal, so `==` would report
failure on correct results. The tolerance is a parameter with a sensible
default rather than a hidden constant.

### 2.10 Two independent routes to the same number

`TogglePrice()` prices the counterpart from the Black-Scholes formula;
`ParityPrice()` derives it from the no-arbitrage relation. These are
genuinely different calculations, so their agreement is evidence that
both are implemented correctly. The test program checks both against the
reference figures.

Note that parity is an identity, not a model result — it holds whatever
process the underlying follows. So agreement tests the implementation,
not the Black-Scholes assumptions.

### 2.11 C++ techniques used

| Technique | Where |
|---|---|
| Abstract base class, pure virtual | `Option::Price()`, `Option::Clone()` |
| Virtual destructor | `Option::~Option()` |
| Template Method pattern | `Option::TogglePrice()` calls virtual `Price()` |
| Prototype pattern | `Clone()` |
| Exception hierarchy with abstract `GetMessage()` | `OptionException` |
| Templates and function objects | `Pricer::Compute` |
| `const` correctness | every selector; `const OptionData&` arguments |
| Initialiser lists throughout | every constructor |
| Self-assignment guards | every `operator=` |
| STL | `std::vector` everywhere, no hand-rolled arrays |
| Boost | `boost::math::normal_distribution` |
| Smart pointers | `std::unique_ptr` from `Clone()` |

---

## 3. European options

### 3.1 Call and put prices for four reference cases

Every value matches the reference prices.

| Case | Call computed | Call expected | Put computed | Put expected |
|---|---|---|---|---|
| 1 | 2.133368 | 2.13337 | 5.846282 | 5.84628 |
| 2 | 7.965567 | 7.96557 | 7.965567 | 7.96557 |
| 3 | 0.204058 | 0.204058 | 4.073262 | 4.07326 |
| 4 | 92.175704 | 92.17570 | 1.247499 | 1.24750 |

Case 2 has `r = 0` and `S = K`, so the call and put are equal — parity
reduces to `C = P` when the discount factor is 1 and the option is at the
money. The implementation reproduces this without being told.

### 3.2 Put-call parity

Parity is implemented both ways.

*As a calculator* — `ParityPrice()` rearranges `C + Ke^(-rT) = P + S` for
whichever price is wanted:

| Case | Put from call | Expected | Call from put | Expected |
|---|---|---|---|---|
| 1 | 5.846282 | 5.84628 | 2.133368 | 2.13337 |
| 2 | 7.965567 | 7.96557 | 7.965567 | 7.96557 |
| 3 | 4.073262 | 4.07326 | 0.204058 | 0.204058 |
| 4 | 1.247499 | 1.24750 | 92.175704 | 92.17570 |

Each parity-derived price matches the Black-Scholes price computed
independently in section 3.1.

*As a check* — `CheckParity(call, put, tolerance)` tests whether a given
pair satisfies the relation. All four cases pass with their own prices
and with the reference figures. The test program also feeds it a
deliberately wrong pair for each case, and it correctly reports failure —
a check that always returns true is no check at all.

### 3.3 Prices over a range of S

Mesh from 10 to 50 with h = 1, produced by the global function
`CreateMeshByStep`. 41 points, first 10, last 50.

Using the Case 1 parameters (T = 0.25, K = 65, sig = 0.30, r = 0.08):

| S | Call | Put |
|---|---|---|
| 10 | 0.000000 | 53.712914 |
| 30 | 0.000000 | 33.712914 |
| 50 | 0.189181 | 13.902095 |

The calls are essentially worthless far below a strike of 65 with only
three months to run — correct, not a bug. Three structural properties
were checked across the whole range rather than at single points: the
call price rises monotonically in S, the put falls, and parity holds at
every one of the 41 points to within 1e-9.

### 3.4 Matrix of parameters in, matrix of prices out

`CreateMatrix(base, values, Parameter::X)` builds a vector of complete
parameter sets differing only in the chosen parameter; `Pricer` prices
them. Results from the Case 1 base case:

| Varying | Low | Mid | High |
|---|---|---|---|
| expiry 0.25 → 2.0 | 2.133368 | 7.847875 | 12.166747 |
| volatility 0.1 → 0.5 | 0.173097 | 2.133368 | 4.472438 |
| strike 50 → 90 | 11.322932 | 0.974980 | 0.017541 |
| rate 0 → 0.16 | 1.754981 | 2.133368 | 2.564129 |

All four move in the expected direction: more time and more volatility
are worth more to an option holder, a higher strike is worth less to a
call holder, a higher rate is worth more to a call holder.

Two design points worth noting:

- Varying the rate moves `b` with it, since `b = r` defines the
  Black-Scholes stock model. Leaving `b` behind would silently change
  which model was being priced.
- The same pricer, given a lambda instead of a class, returns vegas
  rather than prices, and `ComputeAs<bool>` returns parity checks. The
  matrix machinery is not tied to prices at all.

---

## 4. Greeks

### 4.1 Exact sensitivities

Data: K = 100, S = 105, T = 0.5, r = 0.1, **b = 0**, sig = 0.36. The
`b = 0` makes this the Black futures model rather than the stock model
used above, which is why the seven-argument `OptionData` constructor
exists.

| Quantity | Computed | Expected |
|---|---|---|
| delta, call | 0.594629 | 0.5946 |
| delta, put | −0.356601 | −0.3566 |
| gamma | 0.013494 | — |
| vega | 26.778123 | — |
| theta, call | −8.396840 | — |
| theta, put | −8.872454 | — |

Gamma and vega are identical for the call and the put, checked to 1e-12.
This is not a coincidence: the terms distinguishing a call from a put are
linear in S, so they vanish on the second derivative, and neither
involves sigma. Signs were checked too — call delta positive, put delta
negative, gamma and vega positive.

The put greeks follow from differentiating the put price, or equivalently
from parity.

### 4.2 Delta over a range of S

The same mesh function as in section 3.3, reused unchanged. Call delta
over S from 10 to 50:

| S | 10 | 30 | 50 |
|---|---|---|---|
| delta | 0.000000 | 0.000002 | 0.004490 |

Delta rises monotonically across all 41 points. This is the same
statement as gamma being positive, so it is a second, independent route
to a result checked in section 4.1.

### 4.3 Matrices of delta and gamma

The same `Pricer::Compute`, given a lambda that returns `Delta()` or
`Gamma()` instead of `Price()`. No new code was needed. Over S from 90 to
120:

| S | 90 | 100 | 120 |
|---|---|---|---|
| delta | 0.368319 | 0.523785 | 0.761485 |
| gamma | 0.015897 | 0.014787 | 0.008704 |

Delta rising and gamma peaking near the money is the expected shape.

### 4.4 Divided differences

```
Delta  =  ( V(S+h) - V(S-h) ) / 2h
Gamma  =  ( V(S+h) - 2V(S) + V(S-h) ) / h^2
```

Exact values for comparison: delta 0.5946286597, gamma 0.0134936371.

| h | delta error | gamma error |
|---|---|---|
| 1e+01 | 4.7e-03 | 8.4e-05 |
| 1e+00 | 4.8e-05 | 8.3e-07 |
| 5e-01 | 1.2e-05 | 2.1e-07 |
| 1e-01 | 4.8e-07 | 8.3e-09 |
| 1e-02 | 4.8e-09 | **4.1e-11** ← best gamma |
| 1e-03 | **4.6e-11** ← best delta | 1.0e-08 |
| 1e-05 | 5.4e-10 | 3.6e-04 |
| 1e-07 | 2.9e-08 | 1.4e+00 |

**The error falls, turns, and rises again.** This is the trade-off between
truncation error and rounding error, made quantitative.

*Falling:* both formulae are second-order accurate, so a tenfold
reduction in h should cut the error a hundredfold. The program computes
the ratio directly: for h from 1.0 to 0.1 it is **100.0**, exactly as
predicted.

*Rising:* the numerators subtract nearly equal numbers. The smaller h
gets, the more leading digits agree and cancel, leaving a result built
from the last few significant digits of the inputs. Dividing that by h or
by h² then magnifies what is left of the rounding noise.

*Why gamma turns first* — at h = 1e-3 rather than 1e-5. Gamma divides by
h² instead of 2h, so the same absolute noise in the numerator is
amplified by a factor 1/h larger. At h = 1e-7, h² = 1e-14 and gamma's
answer is meaningless: 1.42 against a true value of 0.013.

*Practical conclusion:* there is a best h and it is not the smallest one.
Somewhere around 1e-2 to 1e-3 for options priced in the tens, as these
are. The default in `DividedDifferences` is 0.1, which is slightly
conservative — a decade away from the optimum on the safe side, where the
error is still 4.8e-07 and the behaviour is insensitive to the exact
choice.

Over a whole range rather than a single point: with h = 0.1 and S from 90
to 120, the worst delta error is 5.48e-07 and the worst gamma error
2.54e-08.

The class takes a `const Option&`, so it also produces numerical greeks
for a perpetual American option — delta 0.541142, gamma 0.010906 — where
no exact formula is implemented to compare against. That is precisely the
situation the technique exists for.

---

## 5. Perpetual American options

### 5.1 The formulae and the test data

An American option may be exercised at any time, and in general has no
closed-form price. The perpetual option is the exception: with no expiry
the problem loses its dependence on time altogether, and what remains can
be solved exactly.

Test data K = 100, sig = 0.1, r = 0.1, b = 0.02, S = 110:

| | Computed | Expected |
|---|---|---|
| call | 18.503500 | 18.5035 |
| put | 3.031060 | 3.03106 |

Three further properties were checked:

- **The price does not depend on T.** `T` is carried in the data but
  takes no part in the formulae; setting it to 999 gives the same price
  to 1e-12. This is why `PerpetualAmericanOption::Validate()` does not
  require `T > 0` while `EuropeanOption::Validate()` does.
- **r must be positive.** The formulae divide by terms that require it,
  and a perpetual option with no discounting has no finite value. This
  is rejected at construction with a message naming the parameter.
- **A perpetual option dominates a European one.** With the same
  parameters at T = 30 the European call is worth 5.183006 against the
  perpetual 18.503500. An American option can always be exercised at
  expiry, so it is worth at least as much as the European; a perpetual
  one has no expiry to be limited by.

Two guards handle the degenerate cases. `y1 = 1` means the call's holding
cost exactly offsets its growth and the option is worth the stock itself;
`y2 = 0` is the corresponding case for the put. Without them the formulae
would divide by zero.

### 5.2 Prices over a range of S

The same mesh function again, unchanged:

| S | Call | Put |
|---|---|---|
| 10 | 0.008262 | 9034894.535861 |
| 30 | 0.283138 | 9764.831374 |
| 50 | 1.464480 | 407.786801 |

The put values far below the strike look alarming but are correct. A
perpetual American put with S = 10 and K = 100 can be exercised
immediately for 90, and never expires — with sig = 0.1 and b = 0.02 the
underlying drifts upward, so the formula values the option on a path that
in this parameter region is a long way from the exercise region. The
formula is a solution of the time-independent problem and does not
restrict itself to the region where the solution is economically
sensible.

What was checked instead: the call rises monotonically in S, the put
falls monotonically, and **the put is worth at least its intrinsic
value** `K − S` at every point. The last is the economically meaningful
constraint for an American option — it may be exercised at once, so it
can never be worth less than exercising.

### 5.3 Perpetual options through the matrix pricer

No new code. `Pricer::Price<PerpetualAmericanOption>` differs from
`Pricer::Price<EuropeanOption>` only in the template argument.

| Varying | Low | Mid | High |
|---|---|---|---|
| volatility 0.05 → 0.45 | 14.955288 | 30.656336 | 45.648450 |
| cost of carry 0 → 0.08 | 13.193298 | 27.382153 | 61.954296 |

More volatility is worth more, as for a European option. A higher cost of
carry raises the call sharply, which is what the exponent `y1` does as
`b` grows.

---

## 6. Monte Carlo and finite differences

`CompareMethods` prices the four reference puts three ways and measures
each against the closed form. Errors are in brackets.

| Case | Closed form | MC, Euler, 100 steps | MC, exact step | FDM, J = 400 |
|---|---|---|---|---|
| 1 (T = 0.25) | 5.846282 | 5.816390 (−0.0299) | 5.847715 (+0.0014) | 5.845355 (−0.0009) |
| 2 (T = 1) | 7.965567 | 7.911098 (−0.0545) | 7.963314 (−0.0023) | 7.961884 (−0.0037) |
| 3 (T = 1) | 4.073262 | 4.067128 (−0.0061) | 4.071805 (−0.0015) | 4.073226 (−0.00004) |
| 4 (T = 30) | 1.247499 | 1.276718 (+0.0292) | 1.246091 (−0.0014) | 1.245663 (−0.0018) |

**Monte Carlo** uses 100,000 paths and a fixed seed, so runs are
reproducible. Every price comes with its standard error (about 0.019,
0.033, 0.0066 and 0.0078 for the four cases), so it says how far it might
be from the truth. The exact GBM step stays well within one standard
error in every case: it has no discretisation bias, so one step suffices
for a European payoff. The Euler-Maruyama scheme carries a bias that
grows with the step size, and it shows most clearly at T = 30, where 100
steps means steps of 0.3 years and the error is about 3.7 standard
errors.

**Finite differences** use explicit Euler in time with centred
differences in S. The scheme is only conditionally stable, so
`MinimumStableSteps()` computes the smallest number of time steps that
keeps it stable for the grid, from 3,600 for Case 1 to 432,000 for
Case 4. Asking for fewer throws instead of returning a diverged number:

```
Invalid parameter timeSteps = 100: unstable: explicit Euler needs at least 3600 time steps for this grid
```

The upper boundary is placed at 5 times the strike, or 15 times for the
30-year option, where the underlying has a real chance of reaching a
nearer boundary. With those choices every finite-difference error is
below 0.004.

---

## 7. Polymorphism and robustness

### 7.1 Both classes through one base pointer

```
European option [T = 0.25, K = 65, sig = 0.3, r = 0.08, b = 0.08, S = 60, call]
  price 2.133368, counterpart 5.846282
Perpetual American option [T = 1, K = 100, sig = 0.1, r = 0.1, b = 0.02, S = 110, call] (T is not used)
  price 18.503500, counterpart 3.031060
```

`TogglePrice()` is written once in the base class and works for both,
because the `Price()` it calls is virtual — the Template Method pattern.
It also restores the object afterwards, including if `Price()` throws,
which the test checks by pricing again after toggling.

### 7.2 Invalid input is rejected, with a message that names the problem

```
Invalid parameter sig = -0.3: volatility must be positive
Invalid parameter K = 0: strike must be positive
Invalid parameter S = -60: underlying price must be positive
Invalid parameter T = 0: expiry must be positive
Invalid parameter r = 0: a perpetual option needs a positive interest rate
```

Also rejected: a negative mesh size, mismatched vector lengths in the
comparison helpers, and a divided-difference step so large that `S − h`
would be negative. All throw from the `OptionException` hierarchy, so one
`catch (const OptionException&)` handles every kind, and `GetMessage()`
is virtual so the message comes from whichever concrete exception was
thrown.

The exception carries the offending parameter's name and value, so the
catch site reports both without having to guess.

---

## 8. Summary of results against reference values

| Quantity | Expected | Computed |
|---|---|---|
| Case 1 call / put | 2.13337 / 5.84628 | 2.133368 / 5.846282 |
| Case 2 call / put | 7.96557 / 7.96557 | 7.965567 / 7.965567 |
| Case 3 call / put | 0.204058 / 4.07326 | 0.204058 / 4.073262 |
| Case 4 call / put | 92.17570 / 1.24750 | 92.175704 / 1.247499 |
| delta call / put | 0.5946 / −0.3566 | 0.594629 / −0.356601 |
| perpetual call / put | 18.5035 / 3.03106 | 18.503500 / 3.031060 |

**93 checks, 93 passed, 0 failed.**
