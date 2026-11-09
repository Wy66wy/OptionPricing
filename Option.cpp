// Option.cpp
//
// Implementation of the Option base class.

#include "Option.hpp"
#include <sstream>
#include <ostream>

// Constructors and destructor

Option::Option() : m_data()
{
}

// The validation runs here, so an invalid object cannot come into
// existence in the first place.
Option::Option(const OptionData& data) : m_data(data)
{
}

Option::Option(const Option& source) : m_data(source.m_data)
{
}

Option::~Option()
{
}

Option& Option::operator=(const Option& source)
{
    if (this == &source)
    {
        return *this;
    }

    m_data = source.m_data;

    return *this;
}

// Checks that apply to every option. Volatility must be positive for the
// formulae to mean anything, and a strike or a spot of zero or less has no
// financial sense.
void Option::Validate() const
{
    if (m_data.sig <= 0.0)
    {
        throw InvalidParameterException("sig", m_data.sig,
            "volatility must be positive");
    }

    if (m_data.K <= 0.0)
    {
        throw InvalidParameterException("K", m_data.K,
            "strike must be positive");
    }

    if (m_data.S <= 0.0)
    {
        throw InvalidParameterException("S", m_data.S,
            "underlying price must be positive");
    }
}

// Selectors and modifiers

const OptionData& Option::Data() const
{
    return m_data;
}

// Every modifier validates afterwards and puts the old value back if the
// new one is rejected, so a failed set leaves the object as it was.
void Option::Data(const OptionData& newData)
{
    OptionData old = m_data;
    m_data = newData;

    try
    {
        Validate();
    }
    catch (const OptionException&)
    {
        m_data = old;
        throw;
    }
}

double Option::T() const { return m_data.T; }
double Option::K() const { return m_data.K; }
double Option::Sigma() const { return m_data.sig; }
double Option::R() const { return m_data.r; }
double Option::B() const { return m_data.b; }
double Option::S() const { return m_data.S; }
OptionType Option::Type() const { return m_data.type; }

void Option::T(double newT)
{
    OptionData d = m_data; d.T = newT; Data(d);
}

void Option::K(double newK)
{
    OptionData d = m_data; d.K = newK; Data(d);
}

void Option::Sigma(double newSigma)
{
    OptionData d = m_data; d.sig = newSigma; Data(d);
}

void Option::R(double newR)
{
    OptionData d = m_data; d.r = newR; Data(d);
}

void Option::B(double newB)
{
    OptionData d = m_data; d.b = newB; Data(d);
}

void Option::S(double newS)
{
    OptionData d = m_data; d.S = newS; Data(d);
}

void Option::Type(OptionType newType)
{
    m_data.type = newType;
}

void Option::Toggle()
{
    m_data = m_data.Toggle();
}

// Written once here rather than in each derived class. Price() is virtual,
// so this picks up whichever formula belongs to the object: the base class
// supplies the common mechanism and delegates the varying part, which is
// the Template Method pattern.
double Option::TogglePrice() const
{
    // The object is restored before returning, including if Price() throws.
    Option* self = const_cast<Option*>(this);

    self->Toggle();

    double result;
    try
    {
        result = Price();
    }
    catch (...)
    {
        self->Toggle();
        throw;
    }

    self->Toggle();

    return result;
}

std::string Option::ToString() const
{
    return m_data.ToString();
}

std::ostream& operator<<(std::ostream& os, const Option& opt)
{
    os << opt.ToString();

    return os;
}