// CompareMethods.cpp
//
// Prices the same options by all three methods and compares them.
//
// A separate program from Main.cpp, built as its own executable.

#include "OptionData.hpp"
#include "EuropeanOption.hpp"
#include "MonteCarloPricer.hpp"
#include "FiniteDifferencePricer.hpp"
#include "OptionException.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cmath>

namespace
{
    struct Case
    {
        std::string name;
        OptionData data;
    };

    void Row(const std::string& method, double price, double exact,
             const std::string& note = "")
    {
        std::cout << "  " << std::left << std::setw(22) << method
                  << std::right << std::fixed << std::setprecision(6)
                  << std::setw(12) << price
                  << std::setw(13) << std::showpos << price - exact
                  << std::noshowpos
                  << std::setw(10) << std::setprecision(3)
                  << 100.0 * (price - exact) / exact << "%"
                  << "  " << note << std::endl;
    }
}

int main()
{
    std::vector<Case> cases;
    cases.push_back({"Case 1  (T = 0.25)",
        OptionData(0.25,  65.0, 0.30, 0.08,  60.0, OptionType::Put)});
    cases.push_back({"Case 2  (T = 1.0)",
        OptionData(1.00, 100.0, 0.20, 0.00, 100.0, OptionType::Put)});
    cases.push_back({"Case 3  (T = 1.0)",
        OptionData(1.00,  10.0, 0.50, 0.12,   5.0, OptionType::Put)});
    cases.push_back({"Case 4  (T = 30)",
        OptionData(30.0, 100.0, 0.30, 0.08, 100.0, OptionType::Put)});

    MonteCarloPricer mcEuler(100, 100000, Scheme::Euler);
    MonteCarloPricer mcExact(1,   100000, Scheme::Exact);

    try
    {
        for (std::size_t i = 0; i < cases.size(); ++i)
        {
            const OptionData& d = cases[i].data;

            std::cout << "\n=== " << cases[i].name << ", put ===" << std::endl;
            std::cout << "  " << std::left << std::setw(22) << "method"
                      << std::right << std::setw(12) << "price"
                      << std::setw(13) << "error"
                      << std::setw(11) << "rel" << std::endl;

            double exact = EuropeanOption(d).Price();
            Row("closed form", exact, exact);

            MonteCarloResult e = mcEuler.Price(d);
            Row("MC, Euler, NT=100", e.price, exact,
                "SE " + std::to_string(e.se).substr(0, 7));

            MonteCarloResult x = mcExact.Price(d);
            Row("MC, exact, NT=1", x.price, exact,
                "SE " + std::to_string(x.se).substr(0, 7));

            // Long maturities need a wider domain; see the header.
            double smax = (d.T > 10.0) ? 15.0 : 5.0;
            FiniteDifferencePricer fd(400, 1, smax);
            long n = fd.MinimumStableSteps(d);
            FiniteDifferencePricer fdOk(400, n, smax);

            Row("FDM, J=400", fdOk.Price(d), exact,
                "N " + std::to_string(n));
        }

        // The stability check in action.
        std::cout << "\n=== Too few time steps ===" << std::endl;
        try
        {
            FiniteDifferencePricer bad(400, 100);
            bad.Price(cases[0].data);
        }
        catch (const OptionException& ex)
        {
            std::cout << "  " << ex.GetMessage() << std::endl;
        }
    }
    catch (const OptionException& ex)
    {
        std::cout << "Error: " << ex.GetMessage() << std::endl;
        return 1;
    }

    return 0;
}
