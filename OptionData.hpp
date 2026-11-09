// OptionData.hpp
//
// The parameters that define an option, gathered into one type.
//
// A struct rather than a class: these are six independent numbers with no
// invariant binding them together and no behaviour of their own, so there
// is nothing for encapsulation to protect. Passing them as one object
// keeps the pricing interfaces short and makes a matrix of option
// parameters a std::vector<OptionData>.

#ifndef OPTIONDATA_HPP
#define OPTIONDATA_HPP

#include <string>
#include <sstream>

// Whether the option is a call or a put. An enumeration rather than a bool:
// IsCall(true) says nothing at the call site, OptionType::Call does.
enum class OptionType
{
    Call,
    Put
};

struct OptionData
{
    double T;              // Expiry time / maturity, in years
    double K;              // Strike price
    double sig;            // Volatility
    double r;              // Risk-free interest rate
    double b;              // Cost of carry
    double S;              // Current price of the underlying
    OptionType type;       // Call or put

    // Default: a one-year at-the-money call. b = r makes it the
    // Black-Scholes stock option model, which is the most common case.
    OptionData()
        : T(1.0), K(100.0), sig(0.2), r(0.0), b(0.0), S(100.0),
        type(OptionType::Call)
    {
    }

    // Stock option: b takes the value of r, as in the plain
    // Black-Scholes model.
    OptionData(double expiry, double strike, double volatility,
        double rate, double spot, OptionType optionType)
        : T(expiry), K(strike), sig(volatility), r(rate), b(rate), S(spot),
        type(optionType)
    {
    }

    // Full form, for futures (b = 0), dividend-paying stock (b = r - q) and
    // currency options (b = r - R).
    OptionData(double expiry, double strike, double volatility,
        double rate, double costOfCarry, double spot,
        OptionType optionType)
        : T(expiry), K(strike), sig(volatility), r(rate), b(costOfCarry),
        S(spot), type(optionType)
    {
    }

    // The same data with the call/put flag flipped. Returns a copy rather
    // than modifying, so the original is left for comparison - which is
    // what put-call parity work needs.
    OptionData Toggle() const
    {
        OptionData other(*this);
        other.type = (type == OptionType::Call) ? OptionType::Put
            : OptionType::Call;
        return other;
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "T = " << T << ", K = " << K << ", sig = " << sig
            << ", r = " << r << ", b = " << b << ", S = " << S
            << ", " << (type == OptionType::Call ? "call" : "put");
        return ss.str();
    }
};

#endif