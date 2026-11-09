// PerpetualAmericanOption.cpp
//
// Implementation of the perpetual American option.

#include "PerpetualAmericanOption.hpp"

#include <cmath>
#include <sstream>

// Constructors and destructor

PerpetualAmericanOption::PerpetualAmericanOption() : Option()
{
    Validate();
}

PerpetualAmericanOption::PerpetualAmericanOption(const OptionData& data)
    : Option(data)
{
    Validate();
}

PerpetualAmericanOption::PerpetualAmericanOption(
    const PerpetualAmericanOption& source)
    : Option(source)
{
}

PerpetualAmericanOption::~PerpetualAmericanOption()
{
}

PerpetualAmericanOption& PerpetualAmericanOption::operator=(
    const PerpetualAmericanOption& source)
{
    if (this == &source)
    {
        return *this;
    }

    Option::operator=(source);

    return *this;
}

void PerpetualAmericanOption::Validate() const
{
    Option::Validate();

    if (R() <= 0.0)
    {
        throw InvalidParameterException("r", R(),
            "a perpetual option needs a positive interest rate");
    }
}

std::unique_ptr<Option> PerpetualAmericanOption::Clone() const
{
    return std::unique_ptr<Option>(new PerpetualAmericanOption(*this));
}

// The exponents.
//
// y1 = 1/2 - b/sig^2 + sqrt((b/sig^2 - 1/2)^2 + 2r/sig^2)
// y2 = 1/2 - b/sig^2 - sqrt( same )
//
// The shared part is written once in each rather than factored into a
// third function: these are two lines of arithmetic and a helper would
// hide more than it saved.

double PerpetualAmericanOption::Y1() const
{
    double sig2 = Sigma() * Sigma();

    double fac = B() / sig2 - 0.5;
    fac *= fac;

    return 0.5 - B() / sig2 + std::sqrt(fac + 2.0 * R() / sig2);
}

double PerpetualAmericanOption::Y2() const
{
    double sig2 = Sigma() * Sigma();

    double fac = B() / sig2 - 0.5;
    fac *= fac;

    return 0.5 - B() / sig2 - std::sqrt(fac + 2.0 * R() / sig2);
}

// Price
//
//        K     ( y1 - 1   S  ) ^ y1                 K     ( y2 - 1   S  ) ^ y2
//  C = ------- ( ------- --- )              P = --------- ( ------- --- )
//      y1 - 1  (   y1     K  )                   1 - y2   (   y2     K  )
//
// The two guards handle the degenerate cases where a denominator vanishes.
// y1 = 1 means the call's holding cost exactly offsets its growth and the
// option is worth the stock itself; y2 = 0 is the corresponding case for
// the put. Without the guards these would divide by zero and produce an
// infinity rather than a price.
double PerpetualAmericanOption::Price() const
{
    if (Type() == OptionType::Call)
    {
        double y1 = Y1();

        if (y1 == 1.0)
        {
            return S();
        }

        double factor = ((y1 - 1.0) * S()) / (y1 * K());

        return K() * std::pow(factor, y1) / (y1 - 1.0);
    }

    double y2 = Y2();

    if (y2 == 0.0)
    {
        return S();
    }

    double factor = ((y2 - 1.0) * S()) / (y2 * K());

    return K() * std::pow(factor, y2) / (1.0 - y2);
}

std::string PerpetualAmericanOption::ToString() const
{
    std::stringstream ss;
    ss << "Perpetual American option [" << Option::ToString()
        << "] (T is not used)";
    return ss.str();
}