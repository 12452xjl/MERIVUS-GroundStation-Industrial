#include "FleetMissionDwellEncoding.h"

#include <algorithm>
#include <cfenv>
#include <cmath>
#include <limits>

namespace {
constexpr double MaximumSeconds = 86400.0;

bool supportedArithmetic()
{
    return std::numeric_limits<float>::is_iec559
        && std::numeric_limits<float>::radix == 2 && std::numeric_limits<float>::digits == 24
        && std::numeric_limits<float>::has_denorm == std::denorm_present
        && std::numeric_limits<double>::is_iec559
        && std::numeric_limits<double>::radix == 2 && std::numeric_limits<double>::digits == 53
        && std::numeric_limits<double>::has_denorm == std::denorm_present
        && std::numeric_limits<double>::round_style == std::round_to_nearest
        && std::fegetround() == FE_TONEAREST;
}

bool number(const QVariant& value, double& result)
{
    if (!value.isValid() || value.isNull()) return false;
    switch (value.type()) {
    case QVariant::Int: case QVariant::UInt: case QVariant::LongLong:
    case QVariant::ULongLong: case QVariant::Double: break;
    default: return false;
    }
    result = value.toDouble();
    return std::isfinite(result) && result >= 0.0 && result <= MaximumSeconds;
}
} // namespace

bool FleetMissionDwellEncoding::encode(const QVariant& request, Value& value)
{
    value = Value{};
    double requested = 0.0;
    if (!supportedArithmetic() || !number(request, requested)) return false;
    const float uploaded = static_cast<float>(requested);
    if (!std::isfinite(uploaded)) return false;
    Value candidate;
    candidate.requestedSeconds = requested;
    candidate.uploadSeconds = static_cast<double>(uploaded);
    if (requested == 0.0) {
        // Zero remains zero; do not invent a positive wait or rounding cost.
        value = candidate;
        return true;
    }
    if (uploaded < std::numeric_limits<float>::epsilon()) return false;
    // Match the declared source-level rotary-wing NAV wait conversion: the
    // multiplication is float32, then the positive result truncates to u64.
    const float microseconds = uploaded * 1000000.0f;
    // The accepted seconds domain bounds this product below 2^38, far below
    // the conversion limit. Check that safe integer bound before the cast.
    constexpr double MaximumSafeModelProduct = 274877906944.0; // 2^38
    if (!std::isfinite(microseconds) || microseconds < 1.0f
        || static_cast<double>(microseconds) >= MaximumSafeModelProduct) return false;
    candidate.modelWaitMicroseconds = static_cast<quint64>(microseconds);
    if (candidate.modelWaitMicroseconds == 0) return false;
    // This u64 is below 2^38 and is therefore exactly representable in
    // binary64. A correctly rounded fused residual determines the quotient's
    // side of the exact ratio; this residual cannot underflow in this domain.
    // Do not expand exact integer-second waits and reject their exact windows.
    double modelSeconds = static_cast<double>(candidate.modelWaitMicroseconds) / 1000000.0;
    const double quotientResidual = std::fma(modelSeconds, 1000000.0,
        -static_cast<double>(candidate.modelWaitMicroseconds));
    if (quotientResidual < 0.0)
        modelSeconds = std::nextafter(modelSeconds, std::numeric_limits<double>::infinity());
    candidate.budgetSeconds = std::max(requested, std::max(candidate.uploadSeconds, modelSeconds));
    if (!std::isfinite(candidate.budgetSeconds)) return false;
    value = candidate;
    return true;
}

bool FleetMissionDwellEncoding::addBudget(double current, double addition, double& total)
{
    total = 0.0;
    if (!supportedArithmetic() || !std::isfinite(current) || !std::isfinite(addition)
        || current < 0.0 || addition < 0.0) return false;
    if (current == 0.0) { total = addition; return true; }
    if (addition == 0.0) { total = current; return true; }
    const double sum = current + addition;
    if (!std::isfinite(sum)) return false;
    // Knuth TwoSum in binary64 RN, with finite nonnegative operands and sum:
    // exact(current + addition) == sum + residual. Preserve exact/upward sums.
    const double virtualAddition = sum - current;
    const double currentError = current - (sum - virtualAddition);
    const double additionError = addition - virtualAddition;
    const double residual = currentError + additionError;
    const double upper = residual > 0.0
        ? std::nextafter(sum, std::numeric_limits<double>::infinity()) : sum;
    if (!std::isfinite(upper)) return false;
    total = upper;
    return true;
}
