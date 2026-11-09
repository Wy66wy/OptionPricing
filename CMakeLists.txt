cmake_minimum_required(VERSION 3.15)
project(OptionPricing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Boost is used header-only (Boost.Math, for the normal distribution).
# If it is not found automatically, pass -DBOOST_ROOT=<folder containing boost/>
# or set a BOOST_ROOT environment variable.
find_path(BOOST_INCLUDE_DIR boost/math/distributions/normal.hpp
          HINTS ${BOOST_ROOT} $ENV{BOOST_ROOT})
if(NOT BOOST_INCLUDE_DIR)
    message(FATAL_ERROR
        "Boost headers not found. Install Boost, or set BOOST_ROOT to the "
        "folder that contains the boost/ directory.")
endif()

add_library(pricing STATIC
    Option.cpp
    EuropeanOption.cpp
    Perpetualamericanoption.cpp
    MeshGenerator.cpp
    DividedDifferences.cpp
    MonteCarloPricer.cpp
    FiniteDifferencePricer.cpp)
target_include_directories(pricing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pricing SYSTEM PUBLIC ${BOOST_INCLUDE_DIR})

if(NOT MSVC)
    target_compile_options(pricing PUBLIC -Wall -Wextra -Wnon-virtual-dtor -pedantic)
endif()

# The test program: every public function, checked against reference values.
add_executable(OptionPricing Main.cpp)
target_link_libraries(OptionPricing PRIVATE pricing)

# Closed form, Monte Carlo and finite differences side by side.
add_executable(CompareMethods CompareMethods.cpp)
target_link_libraries(CompareMethods PRIVATE pricing)

enable_testing()
add_test(NAME checks COMMAND OptionPricing)
