// EuropeanOption.cpp
//
// Implementation of the European option.

#include "EuropeanOption.hpp"

#include <boost/math/distributions/normal.hpp>

#include <cmath>
#include <sstream>

// The standard normal distribution, N(0,1). Boost supplies both the
// cumulative distribution function N(x) and the density n(x), so neither
// has to be written here or approximated.
namespace
{
    const boost::math::normal_distribution<double> standardNormal(0.0, 1.0);

    // N(x), the cumulative distribution function
    double N(double x)
    {
        return boost::math::cdf(standardNormal, x);
    }

    // n(x), the probability density function
    double n(double x)
    {
        return boost::math::pdf(standardNormal, x);
    }
}

// Constructors and destructor

EuropeanOption::EuropeanOption() : Option()
{
    Validate();
}

EuropeanOption::EuropeanOption(const OptionData& data) : Option(data)
{
    Validate();
}

EuropeanOption::EuropeanOption(const EuropeanOption& source) : Option(source)
{
}

EuropeanOption::~EuropeanOption()
{
}

EuropeanOption& EuropeanOption::operator=(const EuropeanOption& source)
{
    if (this == &source)
    {
        return *this;
    }

    Option::operator=(source);

    return *this;
}

void EuropeanOption::Validate() const
{
    Option::Validate();

    if (T() <= 0.0)
    {
        throw InvalidParameterException("T", T(),
            "expiry must be positive");
    }
}

// d1 and d2.

double EuropeanOption::D1() const
{
    double tmp = Sigma() * std::sqrt(T());

    return (std::log(S() / K()) + (B() + 0.5 * Sigma() * Sigma()) * T()) / tmp;
}

// d2 = d1 - sig sqrt(T), so it is written that way rather than repeating
// the whole quotient.
double EuropeanOption::D2() const
{
    return D1() - Sigma() * std::sqrt(T());
}

// Price

// The generalised Black-Scholes formula for a call, and its counterpart
// for a put. One function rather than two, with the type deciding - the
// caller asks an option for its price without having to know which kind it
// is holding, which is what lets the matrix pricer treat them uniformly.
double EuropeanOption::Price() const
{
    double d1 = D1();
    double d2 = D2();

    if (Type() == OptionType::Call)
    {
        return S() * std::exp((B() - R()) * T()) * N(d1)
            - K() * std::exp(-R() * T()) * N(d2);
    }

    return K() * std::exp(-R() * T()) * N(-d2)
        - S() * std::exp((B() - R()) * T()) * N(-d1);
}

std::unique_ptr<Option> EuropeanOption::Clone() const
{
    return std::unique_ptr<Option>(new EuropeanOption(*this));
}

// The greeks.
//
// The put versions follow from differentiating the put price, or
// equivalently from parity. Gamma and Vega are the same for both, since
// the terms that differ between a call and a put are linear in S and so
// vanish on the second derivative.

double EuropeanOption::Delta() const
{
    double d1 = D1();

    if (Type() == OptionType::Call)
    {
        return std::exp((B() - R()) * T()) * N(d1);
    }

    return std::exp((B() - R()) * T()) * (N(d1) - 1.0);
}

double EuropeanOption::Gamma() const
{
    double d1 = D1();

    return n(d1) * std::exp((B() - R()) * T())
        / (S() * Sigma() * std::sqrt(T()));
}

double EuropeanOption::Vega() const
{
    double d1 = D1();

    return S() * std::sqrt(T()) * std::exp((B() - R()) * T()) * n(d1);
}

double EuropeanOption::Theta() const
{
    double d1 = D1();
    double d2 = D2();

    double common = -(S() * Sigma() * std::exp((B() - R()) * T()) * n(d1))
        / (2.0 * std::sqrt(T()));

    if (Type() == OptionType::Call)
    {
        return common
            - (B() - R()) * S() * std::exp((B() - R()) * T()) * N(d1)
            - R() * K() * std::exp(-R() * T()) * N(d2);
    }

    return common
        + (B() - R()) * S() * std::exp((B() - R()) * T()) * N(-d1)
        + R() * K() * std::exp(-R() * T()) * N(-d2);
}

// Put-call parity

// C + K exp(-rT) = P + S, rearranged for whichever price is wanted.
//
// Parity is a no-arbitrage identity, not a model result: it holds whatever
// process the underlying follows. So agreement between this and the
// Black-Scholes price is a check on the implementation of the formulae
// rather than a test of the model.
double EuropeanOption::ParityPrice() const
{
    double price = Price();

    if (Type() == OptionType::Call)
    {
        // Given the call, the put is C + K exp(-rT) - S
        return price + K() * std::exp(-R() * T()) - S();
    }

    // Given the put, the call is P + S - K exp(-rT)
    return price + S() - K() * std::exp(-R() * T());
}

bool EuropeanOption::CheckParity(double callPrice, double putPrice,
    double tolerance) const
{
    double left = callPrice + K() * std::exp(-R() * T());
    double right = putPrice + S();

    return std::abs(left - right) < tolerance;
}

bool EuropeanOption::CheckParity(double tolerance) const
{
    double thisPrice = Price();
    double otherPrice = TogglePrice();

    if (Type() == OptionType::Call)
    {
        return CheckParity(thisPrice, otherPrice, tolerance);
    }

    return CheckParity(otherPrice, thisPrice, tolerance);
}

std::string EuropeanOption::ToString() const
{
    std::stringstream ss;
    ss << "European option [" << Option::ToString() << "]";
    return ss.str();
}