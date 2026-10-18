#include "FleetMissionPlanningWorker.h"

#include "FleetMissionCompiler.h"

#include <QJsonDocument>
#include <QMetaType>
#include <cmath>

namespace {
constexpr qint64 MaximumBytes = 32LL * 1024 * 1024;
constexpr int MaximumDepth = 24;
constexpr int MaximumNodes = 1000000;

bool jsonValue(const QVariant& value, int depth, int* nodes, qint64* bytes)
{
    if (depth > MaximumDepth || ++*nodes > MaximumNodes) return false;
    if (!value.isValid() || value.userType() == QMetaType::Nullptr) return true;
    if (value.isNull() && value.userType() != QMetaType::QString) return false;
    if (value.userType() == QMetaType::QVariantMap) {
        const QVariantMap map = value.toMap();
        if (map.size() > MaximumNodes - *nodes) return false;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.key().size() > 256) return false;
            const QByteArray key = it.key().toUtf8();
            if (QString::fromUtf8(key) != it.key()) return false;
            *bytes += key.size();
            if (*bytes > MaximumBytes || !jsonValue(it.value(), depth + 1, nodes, bytes)) return false;
        }
        return true;
    }
    if (value.userType() == QMetaType::QVariantList || value.userType() == QMetaType::QStringList) {
        const QVariantList list = value.toList();
        if (list.size() > MaximumNodes - *nodes) return false;
        for (const QVariant& child : list) if (!jsonValue(child, depth + 1, nodes, bytes)) return false;
        return true;
    }
    if (value.userType() == QMetaType::QString) {
        const QString string = value.toString();
        if (string.size() > MaximumBytes - *bytes) return false;
        const QByteArray utf8 = string.toUtf8();
        *bytes += utf8.size();
        return *bytes <= MaximumBytes && QString::fromUtf8(utf8) == string;
    }
    if (value.userType() == QMetaType::Bool) return true;
    switch (value.userType()) {
    case QMetaType::Int: case QMetaType::UInt:
        return true; // Every 32-bit integer is exactly represented by binary64.
    case QMetaType::LongLong: {
        const qint64 original = value.toLongLong();
        const double encoded = double(original);
        // Check the binary64 cast domain BEFORE casting back: LLONG_MAX
        // rounds to +2^63, whose conversion to qint64 would be undefined.
        return encoded >= -9223372036854775808.0 && encoded < 9223372036854775808.0
            && qint64(encoded) == original;
    }
    case QMetaType::ULongLong: {
        const quint64 original = value.toULongLong();
        const double encoded = double(original);
        // ULLONG_MAX similarly rounds to +2^64; reject before conversion.
        return encoded >= 0.0 && encoded < 18446744073709551616.0
            && quint64(encoded) == original;
    }
    case QMetaType::Double: case QMetaType::Float:
        // A floating input already has a represented value. Its actual
        // business schema, not the transport snapshot, sets magnitude limits.
        return std::isfinite(value.toDouble());
    default: return false;
    }
}
}

FleetMissionPlanningWorker::FleetMissionPlanningWorker(const QVariantMap& task, const QVariantList& fleet,
                                                     const Cancellation& cancellation)
    : QThread(nullptr), _task(task), _fleet(fleet), _cancellation(cancellation)
{
}

bool FleetMissionPlanningWorker::snapshot(const QVariantMap& input, QVariantMap* output)
{
    if (!output) return false;
    const QVariantMap source(input);
    output->clear();
    int nodes = 0;
    qint64 bytes = 0;
    if (!jsonValue(source, 0, &nodes, &bytes)) return false;
    const QJsonDocument document = QJsonDocument::fromVariant(source);
    if (!document.isObject() || document.toJson(QJsonDocument::Compact).size() > MaximumBytes) return false;
    *output = document.toVariant().toMap();
    return true;
}

void FleetMissionPlanningWorker::run()
{
    if (!_cancellation || _cancellation->load(std::memory_order_acquire)) {
        _error = QStringLiteral("mission-planning-cancelled");
        return;
    }
    const QVariantMap result = FleetMissionCompiler::compileCancelable(_task, _fleet, _cancellation);
    if (_cancellation->load(std::memory_order_acquire) || result.value(QStringLiteral("cancelled")).toBool()) {
        _error = QStringLiteral("mission-planning-cancelled");
        return;
    }
    if (result.isEmpty() || !snapshot(result, &_plan)) {
        _error = QStringLiteral("mission-planning-result-resource-or-value-invalid");
        return;
    }
    if (_cancellation->load(std::memory_order_acquire)) {
        _plan.clear();
        _error = QStringLiteral("mission-planning-cancelled");
    }
}
