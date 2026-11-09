// MonteCarloPricer.cpp

#include "MonteCarloPricer.hpp"

#include <cmath>
#include <random>
#include <algorithm>

MonteCarloPricer::MonteCarloPricer(long steps, long paths,
                                   Scheme scheme, std::uint32_t seed)
    : m_steps(steps), m_paths(paths), m_scheme(scheme), m_seed(seed)
{
    if (steps < 1)
    {
        throw InvalidParameterException("steps", static_cast<double>(steps),
                                        "need at least one time step");
    }

    if (paths < 2)
    {
        throw InvalidParameterException("paths", static_cast<double>(paths),
                                        "need at least two paths for an error estimate");
    }
}

MonteCarloResult MonteCarloPricer::Price(const OptionData& d) const
{
    std::mt19937 rng(m_seed);
    std::normal_distribution<double> z(0.0, 1.0);

    const double k = d.T / static_cast<double>(m_steps);
    const double sqrtK = std::sqrt(k);

    // For the exact scheme the drift and diffusion of log S are constant,
    // so they are worked out once rather than on every step.
    const double logDrift = (d.b - 0.5 * d.sig * d.sig) * k;
    const double logVol = d.sig * sqrtK;

    // Running sums for the mean and the variance in one pass.
    double sum = 0.0;
    double sumSquares = 0.0;

    for (long i = 0; i < m_paths; ++i)
    {
        double S = d.S;

        for (long n = 0; n < m_steps; ++n)
        {
            if (m_scheme == Scheme::Euler)
            {
                S += d.b * S * k + d.sig * S * sqrtK * z(rng);
            }
            else
            {
                S *= std::exp(logDrift + logVol * z(rng));
            }
        }

        double payoff = (d.type == OptionType::Call)
                      ? std::max(S - d.K, 0.0)
                      : std::max(d.K - S, 0.0);

        sum += payoff;
        sumSquares += payoff * payoff;
    }

    const double M = static_cast<double>(m_paths);
    const double discount = std::exp(-d.r * d.T);

    double variance = (sumSquares - sum * sum / M) / (M - 1.0);
    if (variance < 0.0)
    {
        variance = 0.0;      // cancellation when payoffs barely vary
    }

    MonteCarloResult result;
    result.price = discount * sum / M;
    result.sd = discount * std::sqrt(variance);
    result.se = result.sd / std::sqrt(M);

    return result;
}
