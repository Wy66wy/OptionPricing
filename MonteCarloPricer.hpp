// MonteCarloPricer.hpp
//
// Prices a European option by simulating paths of the underlying.
//
// Under the risk-neutral measure the underlying follows
//
//     dS = b S dt + sig S dW
//
// where b is the cost of carry. Each path is stepped from 0 to T, the
// payoff is taken at T, and the discounted average of the payoffs is the
// price. The standard deviation and standard error come with it, so every
// price carries a statement of how far it might be from the truth.
//
// Two stepping schemes are offered.
//
// Euler-Maruyama approximates each step linearly. It is the scheme any
// SDE can be solved with, and it carries a time-discretisation bias that
// grows with the step size: for long maturities the steps must be small.
//
// Exact stepping uses the closed-form solution of geometric Brownian
// motion, S(t+k) = S(t) exp((b - sig^2/2) k + sig sqrt(k) z). It has no
// discretisation bias at all, so one step suffices for a European payoff.
// It only exists because this particular SDE can be solved; the Euler
// scheme is kept because most cannot.
//
// The generator is std::mt19937 with an explicit seed: reproducible by
// default, and a different seed gives an independent run.

#ifndef MONTECARLOPRICER_HPP
#define MONTECARLOPRICER_HPP

#include "OptionData.hpp"
#include "OptionException.hpp"

#include <cstdint>

enum class Scheme
{
    Euler,             // Euler-Maruyama: general, biased for large steps
    Exact              // Exact GBM step: no discretisation bias
};

// What a simulation returns. A price alone says nothing about how much to
// trust it, so the two error measures travel with it.
struct MonteCarloResult
{
    double price;      // Discounted mean payoff
    double sd;         // Standard deviation of the discounted payoffs
    double se;         // Standard error of the price, sd / sqrt(M)
};

class MonteCarloPricer
{
private:
    long m_steps;          // NT, time steps per path
    long m_paths;          // NSIM, number of paths
    Scheme m_scheme;
    std::uint32_t m_seed;

public:
    MonteCarloPricer(long steps, long paths,
                     Scheme scheme = Scheme::Euler,
                     std::uint32_t seed = 42);

    MonteCarloResult Price(const OptionData& data) const;
};

#endif
