#ifndef LIANG_HYPHENATION_PATTERNS_H
#define LIANG_HYPHENATION_PATTERNS_H

#include <cstdint>
#include <cstddef>

struct LiangPattern
{
    const char *letters;
    const std::uint8_t *values;
    std::uint8_t values_len;
};

struct LiangHyphenationPatterns
{
    const LiangPattern *patterns;
    size_t count;
};

#endif // LIANG_HYPHENATION_PATTERNS_H
