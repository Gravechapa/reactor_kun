#pragma once
#include <string>
#include <array>
#include <random>

class Signature
{
public:
    Signature();

    std::string_view get_random();
    std::string_view get_end() const;

private:
    static const std::array<std::string, 38> _signatures;

    std::mt19937 _gen;
    std::uniform_int_distribution<> _dis{0, _signatures.size() - 2};
};
