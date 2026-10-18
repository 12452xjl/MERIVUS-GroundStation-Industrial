#include "FleetMissionAltitudeArithmetic.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>

namespace {
constexpr std::size_t WordCount = 36;
constexpr unsigned WordBits = 32;
constexpr std::uint64_t FractionMask = UINT64_C(0x000fffffffffffff);
constexpr std::uint64_t MagnitudeMask = UINT64_C(0x7fffffffffffffff);
constexpr std::uint64_t SignMask = UINT64_C(0x8000000000000000);
constexpr std::uint64_t MaximumMantissa = UINT64_C(0x001fffffffffffff);
constexpr std::uint64_t MaximumValueBits = UINT64_C(0x433fffffffffffff); // 2^53-1.
constexpr std::uint64_t MaximumToleranceBits = UINT64_C(0x408f400000000000); // 1000.
using Magnitude = std::array<std::uint32_t, WordCount>;

// No arithmetic conversion: preserve subnormals and signed zero even when
// the host floating-point environment flushes arithmetic subnormals to zero.
bool binary64Bits(double value, std::uint64_t* bits)
{
    if (sizeof(double) != sizeof(std::uint64_t)) return false;
    std::memcpy(bits, &value, sizeof(std::uint64_t));
    return true;
}

bool supportedRepresentation()
{
    if (!std::numeric_limits<double>::is_iec559
        || std::numeric_limits<double>::radix != 2
        || std::numeric_limits<double>::digits != 53
        || std::numeric_limits<double>::min_exponent != -1021
        || std::numeric_limits<double>::max_exponent != 1024
        || std::numeric_limits<std::uint32_t>::digits != 32
        || std::numeric_limits<std::uint64_t>::digits != 64) return false;
    // Also reject mixed/nonmatching floating-point and integer byte layouts.
    std::uint64_t bits = 0;
    return binary64Bits(1.0, &bits) && bits == UINT64_C(0x3ff0000000000000)
        && binary64Bits(-0.0, &bits) && bits == SignMask
        && binary64Bits(1000.0, &bits) && bits == MaximumToleranceBits
        && binary64Bits(9007199254740991.0, &bits) && bits == MaximumValueBits;
}

bool addWord(Magnitude* sum, std::size_t index, std::uint32_t word)
{
    std::uint64_t carry = word;
    while (carry != 0) {
        if (index >= WordCount) return false;
        // At most two uint32 magnitudes; the temporary needs only 33 bits.
        const std::uint64_t total = std::uint64_t((*sum)[index]) + carry;
        (*sum)[index] = static_cast<std::uint32_t>(total);
        carry = total >> WordBits;
        ++index;
    }
    return true;
}

bool addShifted(Magnitude* sum, std::uint64_t product, unsigned shift)
{
    if (product == 0) return true;
    if (shift >= WordCount * WordBits) return false;
    const std::size_t index = shift >> 5;
    const unsigned offset = shift & 31u;
    // Split before shifting: each shifted uint32 chunk occupies <=63 bits.
    // offset may be zero; no expression ever shifts by a type's width.
    for (unsigned chunkIndex = 0; chunkIndex < 2; ++chunkIndex) {
        const std::uint32_t chunk = static_cast<std::uint32_t>(product >> (chunkIndex * WordBits));
        const std::uint64_t shifted = std::uint64_t(chunk) << offset;
        if (!addWord(sum, index + chunkIndex, static_cast<std::uint32_t>(shifted))
            || !addWord(sum, index + chunkIndex + 1, static_cast<std::uint32_t>(shifted >> WordBits))) return false;
    }
    return true;
}

bool addBinary64Magnitude(Magnitude* sum, std::uint64_t magnitudeBits, unsigned coefficient)
{
    const unsigned exponent = unsigned((magnitudeBits >> 52) & UINT64_C(0x7ff));
    if (exponent == 0x7ff || coefficient > 1000) return false;
    const std::uint64_t mantissa = (magnitudeBits & FractionMask)
        | (exponent != 0 ? UINT64_C(0x0010000000000000) : UINT64_C(0));
    // The checked operands guarantee a product <2^63; no division or
    // unchecked overflowing multiplication is used to establish that bound.
    if (mantissa > MaximumMantissa) return false;
    const std::uint64_t product = mantissa * coefficient;
    // All numbers are integers in units of 2^-1074. For a normal value the
    // hidden bit is present and shift=biasedExponent-1; subnormals use zero.
    return addShifted(sum, product, exponent != 0 ? exponent - 1 : 0);
}

int compare(const Magnitude& left, const Magnitude& right)
{
    for (std::size_t index = WordCount; index != 0; --index) {
        if (left[index - 1] < right[index - 1]) return -1;
        if (left[index - 1] > right[index - 1]) return 1;
    }
    return 0;
}

bool subtract(const Magnitude& greater, const Magnitude& lesser, Magnitude* difference)
{
    std::uint64_t borrow = 0;
    for (std::size_t index = 0; index < WordCount; ++index) {
        const std::uint64_t subtrahend = std::uint64_t(lesser[index]) + borrow;
        const std::uint64_t minuend = greater[index];
        if (minuend >= subtrahend) {
            (*difference)[index] = static_cast<std::uint32_t>(minuend - subtrahend);
            borrow = 0;
        } else {
            (*difference)[index] = static_cast<std::uint32_t>((UINT64_C(1) << WordBits) + minuend - subtrahend);
            borrow = 1;
        }
    }
    return borrow == 0;
}
} // namespace

bool FleetMissionAltitudeArithmetic::absoluteSumAtMost(std::initializer_list<Term> terms,
    double maximum, int maximumScale)
{
    if (!supportedRepresentation() || terms.size() < 1 || terms.size() > 4
        || maximumScale < 1 || maximumScale > 1000) return false;
    std::uint64_t maximumBits = 0;
    if (!binary64Bits(maximum, &maximumBits)) return false;
    const std::uint64_t maximumMagnitude = maximumBits & MagnitudeMask;
    // Ordered positive IEEE encodings provide exact bounds without any
    // floating-point subtraction, division, epsilon or subnormal rounding.
    if (maximumMagnitude > MaximumToleranceBits
        || ((maximumBits & SignMask) != 0 && maximumMagnitude != 0)) return false;
    Magnitude positive{}, negative{}, threshold{}, difference{};
    if (!addBinary64Magnitude(&threshold, maximumMagnitude, unsigned(maximumScale))) return false;
    for (const Term& term : terms) {
        if (term.coefficient < -1000 || term.coefficient > 1000) return false;
        std::uint64_t bits = 0;
        if (!binary64Bits(term.value, &bits)) return false;
        const std::uint64_t magnitude = bits & MagnitudeMask;
        // Validate even a zero-coefficient term; invalid data is not erased.
        if (magnitude > MaximumValueBits) return false;
        const bool subtracting = ((bits & SignMask) != 0) != (term.coefficient < 0);
        const unsigned coefficient = unsigned(term.coefficient < 0 ? -term.coefficient : term.coefficient);
        if (!addBinary64Magnitude(subtracting ? &negative : &positive, magnitude, coefficient)) return false;
    }
    const int ordering = compare(positive, negative);
    if (ordering == 0) return true;
    if (!subtract(ordering > 0 ? positive : negative, ordering > 0 ? negative : positive, &difference)) return false;
    return compare(difference, threshold) <= 0;
}
