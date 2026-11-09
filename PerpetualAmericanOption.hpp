// PerpetualAmericanOption.hpp
//
// A perpetual American option.
//
// An American option may be exercised at any time before expiry, and no
// exact solution is known for the general case. The perpetual option is
// the exception: with no expiry date the problem loses its dependence on
// time altogether, and what is left can be solved in closed form.
//
// So this class has the same parameters as a European option except that
// T plays no part, and it has no exact greeks - there is no expiry to
// differentiate with respect to, and the other sensitivities are left to
// DividedDifferences.

#ifndef PERPETUALAMERICANOPTION_HPP
#define PERPETUALAMERICANOPTION_HPP

#include "Option.hpp"

class PerpetualAmericanOption : public Option
{
private:
    // The exponents in the two formulae. Each is a root of the quadratic
    // that comes out of the time-independent problem, y1 taking the
    // positive square root and y2 the negative.
    double Y1() const;
    double Y2() const;

protected:
    // T is not checked here, unlike in the European case: a perpetual
    // option has no expiry, so whatever value T happens to hold is simply
    // unused. r must be positive, though - the formulae divide by terms
    // that require it and a perpetual option with no discounting has no
    // finite value.
    void Validate() const;

public:
    PerpetualAmericanOption();
    PerpetualAmericanOption(const OptionData& data);
    PerpetualAmericanOption(const PerpetualAmericanOption& source);
    ~PerpetualAmericanOption();

    PerpetualAmericanOption& operator = (const PerpetualAmericanOption& source);

    // The exact price, call or put according to the type held.
    double Price() const;

    std::unique_ptr<Option> Clone() const;

    std::string ToString() const;
};

#endif