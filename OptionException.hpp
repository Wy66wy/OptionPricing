// OptionException.hpp
//
// Exceptions for the option pricing classes.
//
// Both classes are implemented here in the header, as they are short.

#ifndef OPTIONEXCEPTION_HPP
#define OPTIONEXCEPTION_HPP

#include <string>
#include <sstream>

// Base class for every error the pricing classes report. GetMessage() is
// pure virtual, so catching this one base class catches all of them.
class OptionException
{
public:
    virtual ~OptionException() {}

    virtual std::string GetMessage() const = 0;
};

// A parameter outside the range the formulae are defined on. The name of
// the offending parameter and its value travel with the exception, so the
// catch site can report both without having to guess.
class InvalidParameterException : public OptionException
{
private:
    std::string m_name;    // Which parameter
    double m_value;        // What it was set to
    std::string m_reason;  // Why that is not allowed

public:
    InvalidParameterException(const std::string& name, double value,
        const std::string& reason)
        : m_name(name), m_value(value), m_reason(reason)
    {
    }

    std::string GetMessage() const
    {
        std::stringstream ss;
        ss << "Invalid parameter " << m_name << " = " << m_value
            << ": " << m_reason;
        return ss.str();
    }
};

// Two collections that had to match in length did not.
class SizeMismatchException : public OptionException
{
private:
    std::size_t m_left;
    std::size_t m_right;

public:
    SizeMismatchException(std::size_t left, std::size_t right)
        : m_left(left), m_right(right)
    {
    }

    std::string GetMessage() const
    {
        std::stringstream ss;
        ss << "Sizes do not match: " << m_left << " and " << m_right;
        return ss.str();
    }
};

#endif