// Option.hpp
//
// Abstract base class for every kind of option this library prices.
//
// What the two concrete options have in common is their data and the
// obligation to produce a price; how that price is arrived at is entirely
// different between them - one integrates over a finite life, the other
// has no expiry at all - so Price() is pure virtual and nothing more is
// promised here.
//
// The base class exists so that pricing code can be written once against
// Option and work for both, which is what the matrix pricer relies on.

#ifndef OPTION_HPP
#define OPTION_HPP

#include "OptionData.hpp"
#include "OptionException.hpp"

#include <string>
#include <iosfwd>
#include <memory>

class Option
{
private:
    OptionData m_data;     // All the parameters

protected:
    // Derived classes check what their own formulae require. Common checks
    // live here; anything specific is added by the derived class.
    virtual void Validate() const;

public:
    Option();
    Option(const OptionData& data);
    Option(const Option& source);

    // virtual, so deleting through an Option* runs the right destructor.
    virtual ~Option();

    Option& operator = (const Option& source);

    // Reading and writing the parameters. Each setter validates, so an
    // object can never hold a parameter set its formulae cannot handle.
    const OptionData& Data() const;
    void Data(const OptionData& newData);

    double T() const;
    double K() const;
    double Sigma() const;
    double R() const;
    double B() const;
    double S() const;
    OptionType Type() const;

    void T(double newT);
    void K(double newK);
    void Sigma(double newSigma);
    void R(double newR);
    void B(double newB);
    void S(double newS);
    void Type(OptionType newType);

    // Switch between call and put in place.
    void Toggle();

    // The price of the option. Pure virtual: a bare Option has no formula.
    virtual double Price() const = 0;

    // A copy of this option, of whatever concrete class it really is.
    //
    // Needed because code written against Option& cannot build another
    // one: it does not know which class it is holding. The object itself
    // does, so it is asked to do the copying. This is the Prototype
    // pattern, and the divided-difference class relies on it to reprice
    // at a shifted underlying without disturbing the original.
    //
    // The caller owns the result. It is returned as a unique_ptr so that
    // ownership is explicit and the object is released even if the code
    // that follows throws.
    virtual std::unique_ptr<Option> Clone() const = 0;

    // The price of the same option with the call/put flag flipped. Every
    // option has a counterpart, so this can be written once here in terms
    // of the virtual Price().
    double TogglePrice() const;

    virtual std::string ToString() const;

    friend std::ostream& operator << (std::ostream& os, const Option& opt);
};

#endif