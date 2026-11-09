// EuropeanOption.hpp
//
// A plain European option priced by the generalised Black-Scholes formula,
// together with its sensitivities and the put-call parity relations.

#ifndef EUROPEANOPTION_HPP
#define EUROPEANOPTION_HPP

#include "Option.hpp"

class EuropeanOption : public Option
{
private:
    // d1 and d2 appear in every formula in this class, so they are computed
    // in one place. Private because they are an artefact of the derivation
    // rather than something a client of an option would ask for.
    double D1() const;
    double D2() const;

protected:
    // Adds the check that the base class cannot make: these formulae
    // integrate over a finite life, so a non-positive expiry is meaningless.
    void Validate() const;

public:
    EuropeanOption();
    EuropeanOption(const OptionData& data);
    EuropeanOption(const EuropeanOption& source);
    ~EuropeanOption();

    EuropeanOption& operator = (const EuropeanOption& source);

    // The exact price, call or put according to the type held.
    double Price() const;

    std::unique_ptr<Option> Clone() const;

    // The greeks. Each is the partial derivative of the price with respect
    // to one parameter, so each measures how much the price moves when that
    // parameter is nudged.
    double Delta() const;                     // dC/dS
    double Gamma() const;                     // d2C/dS2
    double Vega() const;                      // dC/dsig
    double Theta() const;                     // -dC/dT

    // Put-call parity, C + K exp(-rT) = P + S, used two ways.
    //
    // ParityPrice() rearranges it to produce the counterpart price: given a
    // call it returns the put, and the other way round. Note this is not
    // the same calculation as TogglePrice(), which prices the counterpart
    // from the Black-Scholes formula directly - comparing the two is a
    // check on both.
    double ParityPrice() const;

    // CheckParity() asks whether a given pair of prices satisfies the
    // relation, within a tolerance. A tolerance is unavoidable: the two
    // sides are floating point numbers arrived at by different routes and
    // will rarely be bit-for-bit equal.
    bool CheckParity(double callPrice, double putPrice,
        double tolerance = 1.0e-6) const;

    // Whether this option's own price and that of its counterpart satisfy
    // parity. A self-consistency test of the implementation.
    bool CheckParity(double tolerance = 1.0e-6) const;

    std::string ToString() const;
};

#endif