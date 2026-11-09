// Main.cpp
//
// Test program for the option pricing classes.
//
// Every public function of every class is exercised here, and every
// expected value is checked automatically rather than left for the reader
// to compare by eye. A failing check prints FAIL and is counted; the
// totals appear at the end.

#include "OptionData.hpp"
#include "OptionException.hpp"
#include "Option.hpp"
#include "EuropeanOption.hpp"
#include "PerpetualAmericanOption.hpp"
#include "MeshGenerator.hpp"
#include "Pricer.hpp"
#include "DividedDifferences.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>

namespace
{
    int checksRun = 0;
    int checksFailed = 0;

    // Compare a computed value against an expected one and report. The
    // tolerance is explicit because the reference values are quoted to
    // five or six figures, not to machine precision.
    void Check(const std::string& what, double got, double expected,
        double tolerance = 1.0e-4)
    {
        ++checksRun;

        bool ok = std::abs(got - expected) < tolerance;

        if (!ok)
        {
            ++checksFailed;
        }

        std::cout << "  " << std::left << std::setw(34) << what
            << std::right << std::fixed << std::setprecision(6)
            << std::setw(13) << got
            << "   expected " << std::setw(12) << expected
            << (ok ? "   ok" : "   FAIL") << std::endl;
    }

    void CheckTrue(const std::string& what, bool got)
    {
        ++checksRun;

        if (!got)
        {
            ++checksFailed;
        }

        std::cout << "  " << std::left << std::setw(34) << what
            << (got ? "true " : "false")
            << (got ? "   ok" : "   FAIL") << std::endl;
    }

    void Heading(const std::string& text)
    {
        std::cout << "\n=== " << text << " ===" << std::endl;
    }

    void PrintRange(const std::string& label,
        const std::vector<double>& mesh,
        const std::vector<double>& values)
    {
        std::cout << "  " << label << ": ";

        // Only the ends and the middle are printed: forty-one numbers in
        // a row say less than three well-chosen ones.
        std::size_t last = values.size() - 1;
        std::size_t mid = last / 2;

        std::cout << std::fixed << std::setprecision(6)
            << "S=" << mesh[0] << " -> " << values[0] << ",  "
            << "S=" << mesh[mid] << " -> " << values[mid] << ",  "
            << "S=" << mesh[last] << " -> " << values[last]
            << "   (" << values.size() << " points)" << std::endl;
    }
}

int main()
{
    std::cout << "Exact pricing methods" << std::endl;

    // ----------------------------------------------------------------
    // Four test cases with known reference prices.
    // ----------------------------------------------------------------
    struct Case
    {
        std::string name;
        double T, K, sig, r, S;
        double expectedCall;
        double expectedPut;
    };

    std::vector<Case> cases;
    cases.push_back({ "Case 1", 0.25,  65.0, 0.30, 0.08,  60.0,  2.13337, 5.84628 });// T, K, sig, r, S, call, put
    cases.push_back({ "Case 2", 1.00, 100.0, 0.20, 0.00, 100.0,  7.96557, 7.96557 });
    cases.push_back({ "Case 3", 1.00,  10.0, 0.50, 0.12,   5.0,  0.204058, 4.07326 });
    cases.push_back({ "Case 4", 30.0, 100.0, 0.30, 0.08, 100.0, 92.17570, 1.24750 });

    // ----------------------------------------------------------------
    // Call and put prices for the four cases.
    // ----------------------------------------------------------------
    Heading("European call and put prices");

    for (std::size_t j = 0; j < cases.size(); ++j)
    {
        const Case& b = cases[j];

        // b = r for a stock option, which this constructor applies.
        OptionData callData(b.T, b.K, b.sig, b.r, b.S, OptionType::Call);

        EuropeanOption call(callData);
        EuropeanOption put(callData.Toggle());

        Check(b.name + " call", call.Price(), b.expectedCall);
        Check(b.name + " put", put.Price(), b.expectedPut);
    }

    // ----------------------------------------------------------------
    // Put-call parity, both ways round.
    //
    // ParityPrice() computes the counterpart price from the relation;
    // CheckParity() tests whether a given pair satisfies it. The first is
    // a calculation, the second a verification.
    // ----------------------------------------------------------------
    Heading("Put-call parity as a calculator");

    for (std::size_t j = 0; j < cases.size(); ++j)
    {
        const Case& b = cases[j];

        OptionData callData(b.T, b.K, b.sig, b.r, b.S, OptionType::Call);

        EuropeanOption call(callData);
        EuropeanOption put(callData.Toggle());

        // Given the call, parity gives the put, and vice versa. Both are
        // compared against the Black-Scholes values above: the two routes
        // are independent, so agreement checks both.
        Check(b.name + " put from call price", call.ParityPrice(), b.expectedPut);
        Check(b.name + " call from put price", put.ParityPrice(), b.expectedCall);
    }

    Heading("Put-call parity as a check");

    for (std::size_t j = 0; j < cases.size(); ++j)
    {
        const Case& b = cases[j];

        OptionData callData(b.T, b.K, b.sig, b.r, b.S, OptionType::Call);
        EuropeanOption call(callData);

        CheckTrue(b.name + " own prices satisfy parity", call.CheckParity());

        // The reference figures, checked as an outside pair.
        CheckTrue(b.name + " reference pair satisfies",
            call.CheckParity(b.expectedCall, b.expectedPut, 1.0e-4));

        // A pair that should not satisfy it, so the check is shown to be
        // capable of saying no.
        CheckTrue(b.name + " a wrong pair is rejected",
            !call.CheckParity(b.expectedCall, b.expectedPut + 1.0, 1.0e-4));
    }

    // ----------------------------------------------------------------
    // Prices over a range of underlying values.
    // ----------------------------------------------------------------
    Heading("Prices over a mesh of S from 10 to 50, h = 1");

    {
        const Case& b = cases[0];
        OptionData base(b.T, b.K, b.sig, b.r, b.S, OptionType::Call);

        std::vector<double> mesh = CreateMeshByStep(10.0, 50.0, 1.0);

        Check("mesh size", static_cast<double>(mesh.size()), 41.0, 0.5);
        Check("mesh first point", mesh.front(), 10.0);
        Check("mesh last point", mesh.back(), 50.0);

        std::vector<OptionData> matrix = CreateMatrix(base, mesh, Parameter::S);
        std::vector<double> callPrices = Pricer::Price<EuropeanOption>(matrix);

        PrintRange("call prices", mesh, callPrices);

        std::vector<OptionData> putMatrix =
            CreateMatrix(base.Toggle(), mesh, Parameter::S);
        std::vector<double> putPrices = Pricer::Price<EuropeanOption>(putMatrix);

        PrintRange("put prices ", mesh, putPrices);

        // A call is worth more the higher the underlying, a put less.
        // Monotonicity is a property of the formulae, so checking it
        // tests the whole range rather than one point of it.
        bool callRising = true;
        bool putFalling = true;

        for (std::size_t j = 1; j < callPrices.size(); ++j)
        {
            if (callPrices[j] < callPrices[j - 1]) callRising = false;
            if (putPrices[j] > putPrices[j - 1]) putFalling = false;
        }

        CheckTrue("call price rises with S", callRising);
        CheckTrue("put price falls with S", putFalling);

        // At S = K the call and put of a zero-rate option are equal, and
        // here r is not zero, so parity rather than equality is checked
        // across the whole range.
        bool parityHolds = true;

        for (std::size_t j = 0; j < matrix.size(); ++j)
        {
            EuropeanOption opt(matrix[j]);

            if (!opt.CheckParity(callPrices[j], putPrices[j], 1.0e-9))
            {
                parityHolds = false;
            }
        }

        CheckTrue("parity holds across the range", parityHolds);
    }

    // ----------------------------------------------------------------
    // A matrix of parameters in, a matrix of prices out.
    //
    // The same pricer serves every parameter, because what varies is
    // decided when the matrix is built, not when it is priced.
    // ----------------------------------------------------------------
    Heading("Varying each parameter in turn");

    {
        const Case& b = cases[0];
        OptionData base(b.T, b.K, b.sig, b.r, b.S, OptionType::Call);

        // i) expiry
        std::vector<double> expiries = CreateMesh(5, 0.25, 2.0);
        std::vector<double> byExpiry =
            Pricer::Price<EuropeanOption>(CreateMatrix(base, expiries, Parameter::T));
        PrintRange("by expiry  ", expiries, byExpiry);

        // ii) volatility
        std::vector<double> vols = CreateMesh(5, 0.10, 0.50);
        std::vector<double> byVol =
            Pricer::Price<EuropeanOption>(CreateMatrix(base, vols, Parameter::Sigma));
        PrintRange("by vol     ", vols, byVol);

        // iii) any other parameter - the strike and the rate here
        std::vector<double> strikes = CreateMesh(5, 50.0, 90.0);
        std::vector<double> byStrike =
            Pricer::Price<EuropeanOption>(CreateMatrix(base, strikes, Parameter::K));
        PrintRange("by strike  ", strikes, byStrike);

        std::vector<double> rates = CreateMesh(5, 0.0, 0.16);
        std::vector<double> byRate =
            Pricer::Price<EuropeanOption>(CreateMatrix(base, rates, Parameter::R));
        PrintRange("by rate    ", rates, byRate);

        // More time and more volatility are each worth more to an option
        // holder; a higher strike is worth less to a call holder.
        CheckTrue("longer expiry is worth more",
            byExpiry.back() > byExpiry.front());
        CheckTrue("more volatility is worth more",
            byVol.back() > byVol.front());
        CheckTrue("higher strike is worth less to a call",
            byStrike.back() < byStrike.front());

        // The same pricer, given a function instead of a class, returns
        // something other than a price.
        std::vector<OptionData> volMatrix =
            CreateMatrix(base, vols, Parameter::Sigma);

        std::vector<double> vegas = Pricer::Compute(volMatrix,
            [](const OptionData& d) { return EuropeanOption(d).Vega(); });

        PrintRange("vega by vol", vols, vegas);

        // And a result that is not a double at all.
        std::vector<bool> parities = Pricer::ComputeAs<bool>(volMatrix,
            [](const OptionData& d) { return EuropeanOption(d).CheckParity(); });

        bool allHold = true;
        for (std::size_t j = 0; j < parities.size(); ++j)
        {
            if (!parities[j]) allHold = false;
        }

        CheckTrue("parity holds for every volatility", allHold);
    }

    // ----------------------------------------------------------------
    // Exact sensitivities for a futures option.
    //
    // b = 0 here, which is the Black futures model rather than the stock
    // model used above, so the seven-argument constructor is needed.
    // ----------------------------------------------------------------
    Heading("Exact delta and gamma, b = 0");

    OptionData greekData(0.5, 100.0, 0.36, 0.1, 0.0, 105.0, OptionType::Call); // T K sig r b S

    {
        EuropeanOption call(greekData);
        EuropeanOption put(greekData.Toggle());

        Check("delta, call", call.Delta(), 0.5946);
        Check("delta, put", put.Delta(), -0.3566);

        // Gamma and vega do not depend on the call-put distinction: the
        // terms that differ are linear in S, so they vanish on the second
        // derivative and do not involve sigma.
        Check("gamma, call and put agree",
            call.Gamma() - put.Gamma(), 0.0, 1.0e-12);
        Check("vega, call and put agree",
            call.Vega() - put.Vega(), 0.0, 1.0e-12);

        std::cout << "  (gamma = " << call.Gamma()
            << ", vega = " << call.Vega()
            << ", theta call = " << call.Theta()
            << ", theta put = " << put.Theta() << ")" << std::endl;

        // Delta of a call lies in (0,1) and of a put in (-1,0) - and for
        // a future option both are scaled by exp(-rT), which is why the
        // call delta here is below 1 even deep in the money.
        CheckTrue("call delta is positive", call.Delta() > 0.0);
        CheckTrue("put delta is negative", put.Delta() < 0.0);
        CheckTrue("gamma is positive", call.Gamma() > 0.0);
        CheckTrue("vega is positive", call.Vega() > 0.0);
    }

    // ----------------------------------------------------------------
    // Delta over a range of underlying values.
    // ----------------------------------------------------------------
    Heading("Call delta over a mesh of S");

    {
        std::vector<double> mesh = CreateMeshByStep(10.0, 50.0, 1.0);
        std::vector<OptionData> matrix =
            CreateMatrix(greekData, mesh, Parameter::S);

        std::vector<double> deltas = Pricer::Compute(matrix,
            [](const OptionData& d) { return EuropeanOption(d).Delta(); });

        PrintRange("call delta ", mesh, deltas);

        bool rising = true;
        for (std::size_t j = 1; j < deltas.size(); ++j)
        {
            if (deltas[j] < deltas[j - 1]) rising = false;
        }

        // Delta rising in S is the same statement as gamma being
        // positive, so this is a second route to a result checked above.
        CheckTrue("delta rises with S", rising);
    }

    // ----------------------------------------------------------------
    // Matrices of delta and of gamma.
    // ----------------------------------------------------------------
    Heading("Matrices of delta and gamma");

    {
        std::vector<double> mesh = CreateMeshByStep(90.0, 120.0, 10.0);
        std::vector<OptionData> matrix =
            CreateMatrix(greekData, mesh, Parameter::S);

        std::vector<double> deltas = Pricer::Compute(matrix,
            [](const OptionData& d) { return EuropeanOption(d).Delta(); });

        std::vector<double> gammas = Pricer::Compute(matrix,
            [](const OptionData& d) { return EuropeanOption(d).Gamma(); });

        PrintRange("delta      ", mesh, deltas);
        PrintRange("gamma      ", mesh, gammas);

        Check("delta matrix size", static_cast<double>(deltas.size()), 4.0, 0.5);
        Check("gamma matrix size", static_cast<double>(gammas.size()), 4.0, 0.5);
    }

    // ----------------------------------------------------------------
    // Divided differences, and how the error moves with h.
    // ----------------------------------------------------------------
    Heading("Divided differences against the exact greeks");

    {
        EuropeanOption call(greekData);

        double exactDelta = call.Delta();
        double exactGamma = call.Gamma();

        std::cout << "  exact delta " << std::setprecision(10) << exactDelta
            << ", exact gamma " << exactGamma << std::endl << std::endl;

        std::cout << "          h       delta error       gamma error"
            << std::endl;

        double steps[] = { 10.0, 1.0, 0.5, 0.1, 0.01, 1.0e-3, 1.0e-5, 1.0e-7 };

        double bestDeltaError = 1.0e30;
        double bestDeltaH = 0.0;

        for (std::size_t j = 0; j < sizeof(steps) / sizeof(steps[0]); ++j)
        {
            DividedDifferences dd(steps[j]);

            double deltaError = std::abs(dd.Delta(call) - exactDelta);
            double gammaError = std::abs(dd.Gamma(call) - exactGamma);

            if (deltaError < bestDeltaError)
            {
                bestDeltaError = deltaError;
                bestDeltaH = steps[j];
            }

            std::cout << "  " << std::scientific << std::setprecision(1)
                << std::setw(9) << steps[j]
                << std::setw(18) << deltaError
                << std::setw(18) << gammaError << std::endl;
        }

        std::cout << std::fixed << std::setprecision(6);

        // The approximations are second order, so a tenfold reduction in
        // h should cut the error a hundredfold - while rounding stays out
        // of the way.
        DividedDifferences coarse(1.0);
        DividedDifferences fine(0.1);

        double coarseError = std::abs(coarse.Delta(call) - exactDelta);
        double fineError = std::abs(fine.Delta(call) - exactDelta);

        std::cout << "  order check: error ratio for h 1.0 -> 0.1 is "
            << std::setprecision(1) << coarseError / fineError
            << " (second order predicts 100)" << std::setprecision(6)
            << std::endl;

        CheckTrue("second-order convergence",
            coarseError / fineError > 50.0
            && coarseError / fineError < 200.0);

        // And the other half of the story: past some point smaller h is
        // worse, not better.
        DividedDifferences tiny(1.0e-7);
        CheckTrue("very small h is worse than the best h",
            std::abs(tiny.Delta(call) - exactDelta) > bestDeltaError);

        std::cout << "  best h for delta here was " << std::scientific
            << std::setprecision(1) << bestDeltaH
            << std::fixed << std::setprecision(6) << std::endl;

        // Over a whole range, not just one point.
        std::vector<double> mesh = CreateMeshByStep(90.0, 120.0, 1.0);
        std::vector<OptionData> matrix =
            CreateMatrix(greekData, mesh, Parameter::S);

        std::vector<double> exactDeltas = Pricer::Compute(matrix,
            [](const OptionData& d) { return EuropeanOption(d).Delta(); });

        std::vector<double> exactGammas = Pricer::Compute(matrix,
            [](const OptionData& d) { return EuropeanOption(d).Gamma(); });

        DividedDifferences dd(0.1);

        std::vector<double> approxDeltas = dd.Delta<EuropeanOption>(matrix);
        std::vector<double> approxGammas = dd.Gamma<EuropeanOption>(matrix);

        double worstDelta =
            Pricer::MaxAbsoluteDifference(exactDeltas, approxDeltas);
        double worstGamma =
            Pricer::MaxAbsoluteDifference(exactGammas, approxGammas);

        std::cout << "  over S in [90,120] with h = 0.1, worst delta error "
            << std::scientific << std::setprecision(2) << worstDelta
            << ", worst gamma error " << worstGamma
            << std::fixed << std::setprecision(6) << std::endl;

        CheckTrue("delta approximation is close over the range",
            worstDelta < 1.0e-5);
        CheckTrue("gamma approximation is close over the range",
            worstGamma < 1.0e-5);
    }

    // ----------------------------------------------------------------
    // Perpetual American options.
    // ----------------------------------------------------------------
    Heading("Perpetual American option");

    OptionData perpData(1.0, 100.0, 0.1, 0.1, 0.02, 110.0, OptionType::Call); // T K sig r b S

    {
        PerpetualAmericanOption call(perpData);
        PerpetualAmericanOption put(perpData.Toggle());

        Check("perpetual call", call.Price(), 18.5035);
        Check("perpetual put", put.Price(), 3.03106);

        // An American option may be exercised at any time, including at
        // expiry, so it is worth at least as much as the European option
        // that can only be exercised then. A perpetual option has no
        // expiry at all, so it dominates a European option of any finite
        // life with the same parameters.
        OptionData sameButEuropean = perpData;
        sameButEuropean.T = 30.0;

        EuropeanOption europeanCall(sameButEuropean);

        CheckTrue("perpetual beats a 30-year European",
            call.Price() > europeanCall.Price());

        std::cout << "  (perpetual " << call.Price()
            << " against European " << europeanCall.Price()
            << " at T = 30)" << std::endl;

        // T is carried in the data but takes no part in the formulae.
        OptionData otherExpiry = perpData;
        otherExpiry.T = 999.0;

        PerpetualAmericanOption sameAgain(otherExpiry);

        Check("price is independent of T",
            sameAgain.Price(), call.Price(), 1.0e-12);
    }

    // ----------------------------------------------------------------
    // Perpetual prices over a range of S.
    // ----------------------------------------------------------------
    Heading("Perpetual prices over a mesh of S");

    {
        std::vector<double> mesh = CreateMeshByStep(10.0, 50.0, 1.0);

        std::vector<double> calls = Pricer::Price<PerpetualAmericanOption>(
            CreateMatrix(perpData, mesh, Parameter::S));

        std::vector<double> puts = Pricer::Price<PerpetualAmericanOption>(
            CreateMatrix(perpData.Toggle(), mesh, Parameter::S));

        PrintRange("perp calls ", mesh, calls);
        PrintRange("perp puts  ", mesh, puts);

        bool callRising = true;
        bool putFalling = true;

        for (std::size_t j = 1; j < calls.size(); ++j)
        {
            if (calls[j] < calls[j - 1]) callRising = false;
            if (puts[j] > puts[j - 1]) putFalling = false;
        }

        CheckTrue("perpetual call rises with S", callRising);
        CheckTrue("perpetual put falls with S", putFalling);

        // A perpetual American put is worth at least its intrinsic value,
        // since it may be exercised at once.
        bool aboveIntrinsic = true;

        for (std::size_t j = 0; j < mesh.size(); ++j)
        {
            double intrinsic = perpData.K - mesh[j];

            if (intrinsic > 0.0 && puts[j] < intrinsic - 1.0e-9)
            {
                aboveIntrinsic = false;
            }
        }

        CheckTrue("perpetual put beats intrinsic value", aboveIntrinsic);
    }

    // ----------------------------------------------------------------
    // Perpetual prices through the same matrix pricer.
    // ----------------------------------------------------------------
    Heading("Perpetual options through the matrix pricer");

    {
        std::vector<double> vols = CreateMesh(5, 0.05, 0.45);

        std::vector<double> byVol = Pricer::Price<PerpetualAmericanOption>(
            CreateMatrix(perpData, vols, Parameter::Sigma));

        PrintRange("perp by vol", vols, byVol);

        std::vector<double> carries = CreateMesh(5, 0.0, 0.08);

        std::vector<double> byCarry = Pricer::Price<PerpetualAmericanOption>(
            CreateMatrix(perpData, carries, Parameter::B));

        PrintRange("perp by b  ", carries, byCarry);

        CheckTrue("more volatility is worth more",
            byVol.back() > byVol.front());

        // Divided differences work on a perpetual option too, through
        // the base class - the class was never told which option it had.
        DividedDifferences dd(0.1);
        PerpetualAmericanOption call(perpData);

        std::cout << "  approximated delta " << dd.Delta(call)
            << ", gamma " << dd.Gamma(call)
            << "   (no exact formula to compare against)" << std::endl;

        CheckTrue("perpetual delta is between 0 and 1",
            dd.Delta(call) > 0.0 && dd.Delta(call) < 1.0);
    }

    // ----------------------------------------------------------------
    // Polymorphism: both option classes handled through Option*.
    // ----------------------------------------------------------------
    Heading("Polymorphism through the base class");

    {
        std::vector<Option*> options;
        options.push_back(new EuropeanOption(
            OptionData(0.25, 65.0, 0.30, 0.08, 60.0, OptionType::Call)));
        options.push_back(new PerpetualAmericanOption(perpData));

        for (std::size_t j = 0; j < options.size(); ++j)
        {
            std::cout << "  " << options[j]->ToString() << std::endl;
            std::cout << "    price " << options[j]->Price()
                << ", counterpart " << options[j]->TogglePrice()
                << std::endl;
        }

        Check("European through base pointer", options[0]->Price(), 2.13337);
        Check("perpetual through base pointer", options[1]->Price(), 18.5035);

        // TogglePrice is written once in the base class and works for
        // both, because the Price() it calls is virtual.
        Check("European counterpart", options[0]->TogglePrice(), 5.84628);
        Check("perpetual counterpart", options[1]->TogglePrice(), 3.03106);

        // Toggling twice returns the original, so the base class leaves
        // the object as it found it.
        Check("toggling is reversible",
            options[0]->Price(), 2.13337);

        for (std::size_t j = 0; j < options.size(); ++j)
        {
            delete options[j];
        }
    }

    // ----------------------------------------------------------------
    // Accessors and modifiers.
    // ----------------------------------------------------------------
    Heading("Accessors, modifiers and the copy operations");

    {
        OptionData d(0.25, 65.0, 0.30, 0.08, 60.0, OptionType::Call);
        EuropeanOption opt(d);

        Check("T()", opt.T(), 0.25);
        Check("K()", opt.K(), 65.0);
        Check("Sigma()", opt.Sigma(), 0.30);
        Check("R()", opt.R(), 0.08);
        Check("B() equals R() for a stock", opt.B(), 0.08);
        Check("S()", opt.S(), 60.0);
        CheckTrue("Type() is call", opt.Type() == OptionType::Call);

        opt.S(70.0);
        Check("S() after setting", opt.S(), 70.0);

        opt.Toggle();
        CheckTrue("Type() after toggling is put", opt.Type() == OptionType::Put);

        opt.Toggle();
        CheckTrue("toggling back gives a call", opt.Type() == OptionType::Call);

        // Copy construction and assignment, both deep by default here
        // since the class holds no pointers of its own.
        EuropeanOption copy(opt);
        Check("copy has the same price", copy.Price(), opt.Price(), 1.0e-12);

        EuropeanOption assigned(d);
        assigned = opt;
        Check("assignment copies the price", assigned.Price(), opt.Price(), 1.0e-12);

        // Self-assignment leaves the object alone.
        double before = assigned.Price();
        assigned = assigned;
        Check("self-assignment is harmless", assigned.Price(), before, 1.0e-12);

        // Clone produces an object of the right concrete class.
        std::unique_ptr<Option> cloned = opt.Clone();
        Check("clone has the same price", cloned->Price(), opt.Price(), 1.0e-12);

        PerpetualAmericanOption perp(perpData);
        std::unique_ptr<Option> clonedPerp = perp.Clone();
        Check("perpetual clone has the same price",
            clonedPerp->Price(), perp.Price(), 1.0e-12);
    }

    // ----------------------------------------------------------------
    // Error handling.
    // ----------------------------------------------------------------
    Heading("Rejecting invalid parameters");

    {
        // Each of these should throw rather than return a number that
        // looks plausible.
        struct BadCase
        {
            std::string what;
            OptionData data;
        };

        std::vector<BadCase> bad;

        OptionData negativeVol(0.25, 65.0, -0.30, 0.08, 60.0, OptionType::Call);
        OptionData zeroStrike(0.25, 0.0, 0.30, 0.08, 60.0, OptionType::Call);
        OptionData negativeSpot(0.25, 65.0, 0.30, 0.08, -60.0, OptionType::Call);
        OptionData zeroExpiry(0.0, 65.0, 0.30, 0.08, 60.0, OptionType::Call);

        bad.push_back({ "negative volatility", negativeVol });
        bad.push_back({ "zero strike", zeroStrike });
        bad.push_back({ "negative underlying", negativeSpot });
        bad.push_back({ "zero expiry", zeroExpiry });

        for (std::size_t j = 0; j < bad.size(); ++j)
        {
            bool threw = false;
            std::string message;

            try
            {
                EuropeanOption opt(bad[j].data);
            }
            catch (const OptionException& e)
            {
                threw = true;
                message = e.GetMessage();
            }

            CheckTrue(bad[j].what + " is rejected", threw);

            if (threw)
            {
                std::cout << "      " << message << std::endl;
            }
        }

        // A perpetual option does not mind T, since it does not use it,
        // but it does mind a non-positive rate.
        {
            OptionData zeroT = perpData;
            zeroT.T = 0.0;

            bool threw = false;
            try
            {
                PerpetualAmericanOption opt(zeroT);
            }
            catch (const OptionException&)
            {
                threw = true;
            }

            CheckTrue("perpetual accepts T = 0", !threw);
        }

        {
            OptionData zeroRate = perpData;
            zeroRate.r = 0.0;

            bool threw = false;
            std::string message;

            try
            {
                PerpetualAmericanOption opt(zeroRate);
            }
            catch (const OptionException& e)
            {
                threw = true;
                message = e.GetMessage();
            }

            CheckTrue("perpetual rejects a zero rate", threw);

            if (threw)
            {
                std::cout << "      " << message << std::endl;
            }
        }

        // A rejected setter leaves the object as it was.
        {
            EuropeanOption opt(OptionData(0.25, 65.0, 0.30, 0.08, 60.0,
                OptionType::Call));
            double before = opt.Price();

            bool threw = false;
            try
            {
                opt.Sigma(-1.0);
            }
            catch (const OptionException&)
            {
                threw = true;
            }

            CheckTrue("a rejected setter throws", threw);
            Check("the object is unchanged", opt.Price(), before, 1.0e-12);
        }

        // Mesh and matrix errors.
        {
            bool threw = false;
            try
            {
                CreateMeshByStep(10.0, 50.0, -1.0);
            }
            catch (const OptionException&)
            {
                threw = true;
            }

            CheckTrue("a negative mesh size is rejected", threw);
        }

        {
            bool threw = false;
            try
            {
                std::vector<double> a(3, 1.0);
                std::vector<double> b(4, 1.0);
                Pricer::MaxAbsoluteDifference(a, b);
            }
            catch (const OptionException&)
            {
                threw = true;
            }

            CheckTrue("mismatched vector sizes are rejected", threw);
        }

        // A step so large that S - h would be negative.
        {
            EuropeanOption opt(OptionData(0.25, 65.0, 0.30, 0.08, 60.0,
                OptionType::Call));
            DividedDifferences dd(100.0);

            bool threw = false;
            try
            {
                dd.Delta(opt);
            }
            catch (const OptionException&)
            {
                threw = true;
            }

            CheckTrue("too large a difference step is rejected", threw);
        }
    }

    // ----------------------------------------------------------------
    Heading("Totals");

    std::cout << "  " << checksRun << " checks, "
        << (checksRun - checksFailed) << " passed, "
        << checksFailed << " failed" << std::endl;

    return checksFailed == 0 ? 0 : 1;
}
