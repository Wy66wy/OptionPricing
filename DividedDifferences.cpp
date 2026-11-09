// DividedDifferences.cpp
//
// Implementation of the divided difference approximations.

#include "DividedDifferences.hpp"

DividedDifferences::DividedDifferences(double h) : m_h(h)
{
    H(h);
}

double DividedDifferences::H() const
{
    return m_h;
}

void DividedDifferences::H(double newH)
{
    if (newH <= 0.0)
    {
        throw InvalidParameterException("h", newH, "step must be positive");
    }

    m_h = newH;
}

// Delta = ( V(S+h) - V(S-h) ) / 2h
//
// The option is priced at two shifted values of S. Copies of its data are
// made and new options built from them rather than the original being
// moved and moved back: the original is left untouched, so the function
// is safe to call on a const option and leaves nothing behind if a price
// throws.
double DividedDifferences::Delta(const Option& option) const
{
    OptionData up = option.Data();
    OptionData down = option.Data();

    up.S = option.S() + m_h;
    down.S = option.S() - m_h;

    if (down.S <= 0.0)
    {
        throw InvalidParameterException("h", m_h,
            "step is too large: S - h is not a valid underlying price");
    }

    // The option clones itself, so the right concrete class is rebuilt
    // without this code knowing which one it is.
    std::unique_ptr<Option> shifted = option.Clone();

    shifted->Data(up);
    double priceUp = shifted->Price();

    shifted->Data(down);
    double priceDown = shifted->Price();

    return (priceUp - priceDown) / (2.0 * m_h);
}

// Gamma = ( V(S+h) - 2V(S) + V(S-h) ) / h^2
double DividedDifferences::Gamma(const Option& option) const
{
    OptionData up = option.Data();
    OptionData down = option.Data();

    up.S = option.S() + m_h;
    down.S = option.S() - m_h;

    if (down.S <= 0.0)
    {
        throw InvalidParameterException("h", m_h,
            "step is too large: S - h is not a valid underlying price");
    }

    std::unique_ptr<Option> shifted = option.Clone();

    shifted->Data(up);
    double priceUp = shifted->Price();

    shifted->Data(down);
    double priceDown = shifted->Price();

    double priceMid = option.Price();

    return (priceUp - (2.0 * priceMid) + priceDown) / (m_h * m_h);
}