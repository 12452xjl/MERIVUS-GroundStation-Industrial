#include "FormationPlanner.h"

#include <QSet>
#include <cmath>
#include <QtMath>

namespace {
const double kEpsilon = 1.0e-6;
const double kPi = 3.14159265358979323846;

double mapDouble(const QVariantMap& map, const QString& key, double fallback = 0.0)
{
    const QVariant value = map.value(key);
    return value.isValid() ? value.toDouble() : fallback;
}

QVariantList mapList(const QVariantMap& map, const QString& key)
{
    const QVariant value = map.value(key);
    return value.canConvert<QVariantList>() ? value.toList() : QVariantList();
}

double distanceBetween(const QVariantMap& first, const QVariantMap& second)
{
    const double dx = mapDouble(first, QStringLiteral("x")) - mapDouble(second, QStringLiteral("x"));
    const double dy = mapDouble(first, QStringLiteral("y")) - mapDouble(second, QStringLiteral("y"));
    const double dz = mapDouble(first, QStringLiteral("z")) - mapDouble(second, QStringLiteral("z"));
    return qSqrt(dx * dx + dy * dy + dz * dz);
}
}

FormationPlanner::FormationPlanner(QObject* parent)
    : QObject(parent)
{
}

QString FormationPlanner::formationType() const
{
    return formationTypeToString(_formationType);
}

void FormationPlanner::setFormationType(const QString& type)
{
    const FormationType normalized = formationTypeFromString(type);
    if (_formationType == normalized) {
        return;
    }
    _formationType = normalized;
    emit formationChanged();
    emitPlanChanged();
}

void FormationPlanner::setFormationType(FormationType type)
{
    if (type < Line || type > Custom || _formationType == type) {
        return;
    }
    _formationType = type;
    emit formationChanged();
    emitPlanChanged();
}

void FormationPlanner::setMemberIds(const QVariantList& ids)
{
    QVariantList normalized;
    QSet<QString> seen;
    for (const QVariant& id : ids) {
        const QString key = valueKey(id);
        if (key.isEmpty() || seen.contains(key)) {
            continue;
        }
        seen.insert(key);
        normalized.append(id);
    }
    if (_memberIds == normalized) {
        return;
    }
    _memberIds = normalized;
    emit membersChanged();
    emitPlanChanged();
}

void FormationPlanner::setSpacingMeters(double spacingMeters)
{
    const double normalized = qMax(kEpsilon, spacingMeters);
    if (qFuzzyCompare(_spacingMeters, normalized)) {
        return;
    }
    _spacingMeters = normalized;
    emit parametersChanged();
    emitPlanChanged();
}

void FormationPlanner::setScale(double scale)
{
    const double normalized = qMax(kEpsilon, scale);
    if (qFuzzyCompare(_scale, normalized)) {
        return;
    }
    _scale = normalized;
    emit parametersChanged();
    emitPlanChanged();
}

void FormationPlanner::setRotationDegrees(double rotationDegrees)
{
    double normalized = std::fmod(rotationDegrees, 360.0);
    if (normalized > 180.0) {
        normalized -= 360.0;
    } else if (normalized <= -180.0) {
        normalized += 360.0;
    }
    if (qFuzzyCompare(_rotationDegrees + 1.0, normalized + 1.0)) {
        return;
    }
    _rotationDegrees = normalized;
    emit parametersChanged();
    emitPlanChanged();
}

void FormationPlanner::setReferenceLatitude(double latitude)
{
    if (qFuzzyCompare(_referenceLatitude + 1.0, latitude + 1.0)) {
        return;
    }
    _referenceLatitude = latitude;
    emit referenceChanged();
    emitPlanChanged();
}

void FormationPlanner::setReferenceLongitude(double longitude)
{
    if (qFuzzyCompare(_referenceLongitude + 1.0, longitude + 1.0)) {
        return;
    }
    _referenceLongitude = longitude;
    emit referenceChanged();
    emitPlanChanged();
}

void FormationPlanner::setReferenceAltitude(double altitude)
{
    if (qFuzzyCompare(_referenceAltitude + 1.0, altitude + 1.0)) {
        return;
    }
    _referenceAltitude = altitude;
    emit referenceChanged();
    emitPlanChanged();
}

void FormationPlanner::setReferenceCoordinate(double latitude, double longitude, double altitude)
{
    bool changed = false;
    if (!qFuzzyCompare(_referenceLatitude + 1.0, latitude + 1.0)) {
        _referenceLatitude = latitude;
        changed = true;
    }
    if (!qFuzzyCompare(_referenceLongitude + 1.0, longitude + 1.0)) {
        _referenceLongitude = longitude;
        changed = true;
    }
    if (!qFuzzyCompare(_referenceAltitude + 1.0, altitude + 1.0)) {
        _referenceAltitude = altitude;
        changed = true;
    }
    if (changed) {
        emit referenceChanged();
        emitPlanChanged();
    }
}

void FormationPlanner::setMinimumSpacingMeters(double minimumSpacingMeters)
{
    const double normalized = qMax(0.0, minimumSpacingMeters);
    if (qFuzzyCompare(_minimumSpacingMeters + 1.0, normalized + 1.0)) {
        return;
    }
    _minimumSpacingMeters = normalized;
    emit parametersChanged();
    emitPlanChanged();
}

void FormationPlanner::setCustomOffsets(const QVariantList& offsets)
{
    if (_customOffsets == offsets) {
        return;
    }
    _customOffsets = offsets;
    emit parametersChanged();
    emitPlanChanged();
}

QVariantList FormationPlanner::relativeOffsets() const
{
    return transformedOffsets(baseOffsets());
}

QVariantList FormationPlanner::baseOffsets() const
{
    QVariantList offsets;
    const int count = _memberIds.size();
    if (count <= 0) {
        return offsets;
    }

    if (_formationType == Custom) {
        for (int i = 0; i < count; ++i) {
            const QVariantMap custom = i < _customOffsets.size()
                    ? _customOffsets.at(i).toMap() : QVariantMap();
            offsets.append(offsetMap(_memberIds.at(i), mapDouble(custom, QStringLiteral("x")),
                                     mapDouble(custom, QStringLiteral("y")),
                                     mapDouble(custom, QStringLiteral("z"))));
        }
        return offsets;
    }

    if (_formationType == Circle) {
        const double radius = count > 1
                ? (_spacingMeters / (2.0 * qSin(kPi / static_cast<double>(count))))
                : 0.0;
        for (int i = 0; i < count; ++i) {
            const double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(count);
            offsets.append(offsetMap(_memberIds.at(i), radius * qCos(angle), radius * qSin(angle), 0.0));
        }
        return offsets;
    }

    if (_formationType == Grid) {
        const int columns = qMax(1, static_cast<int>(qCeil(qSqrt(static_cast<double>(count)))));
        const int rows = qCeil(static_cast<double>(count) / static_cast<double>(columns));
        for (int i = 0; i < count; ++i) {
            const int column = i % columns;
            const int row = i / columns;
            const double x = (static_cast<double>(column) - (columns - 1) / 2.0) * _spacingMeters;
            const double y = ((rows - 1) / 2.0 - static_cast<double>(row)) * _spacingMeters;
            offsets.append(offsetMap(_memberIds.at(i), x, y, 0.0));
        }
        return offsets;
    }

    for (int i = 0; i < count; ++i) {
        double x = 0.0;
        double y = 0.0;
        if (_formationType == Column) {
            y = (static_cast<double>(i) - (count - 1) / 2.0) * _spacingMeters;
        } else if (_formationType == Vee || _formationType == Wedge) {
            if (i > 0) {
                const int slot = i;
                const int row = (slot + 1) / 2;
                const double side = (slot % 2 == 0) ? 1.0 : -1.0;
                x = side * row * _spacingMeters;
                y = -row * _spacingMeters;
            }
        } else { // Line
            x = (static_cast<double>(i) - (count - 1) / 2.0) * _spacingMeters;
        }
        offsets.append(offsetMap(_memberIds.at(i), x, y, 0.0));
    }
    return offsets;
}

QVariantList FormationPlanner::transformedOffsets(const QVariantList& offsets) const
{
    QVariantList transformed;
    const double angle = qDegreesToRadians(_rotationDegrees);
    const double cosine = qCos(angle);
    const double sine = qSin(angle);
    for (const QVariant& value : offsets) {
        const QVariantMap source = value.toMap();
        const double x = mapDouble(source, QStringLiteral("x")) * _scale;
        const double y = mapDouble(source, QStringLiteral("y")) * _scale;
        const double z = mapDouble(source, QStringLiteral("z")) * _scale;
        transformed.append(offsetMap(source.value(QStringLiteral("memberId")),
                                     x * cosine - y * sine,
                                     x * sine + y * cosine, z));
    }
    return transformed;
}

QVariantMap FormationPlanner::minimumSpacingReport() const
{
    const QVariantList offsets = relativeOffsets();
    double minimumDistance = -1.0;
    QVariantList violatingPairs;
    int pairCount = 0;
    for (int i = 0; i < offsets.size(); ++i) {
        const QVariantMap first = offsets.at(i).toMap();
        for (int j = i + 1; j < offsets.size(); ++j) {
            const QVariantMap second = offsets.at(j).toMap();
            const double distance = distanceBetween(first, second);
            if (minimumDistance < 0.0 || distance < minimumDistance) {
                minimumDistance = distance;
            }
            ++pairCount;
            if (_minimumSpacingMeters > 0.0 && distance + kEpsilon < _minimumSpacingMeters) {
                QVariantMap pair;
                pair.insert(QStringLiteral("first"), first.value(QStringLiteral("memberId")));
                pair.insert(QStringLiteral("second"), second.value(QStringLiteral("memberId")));
                pair.insert(QStringLiteral("distanceMeters"), distance);
                violatingPairs.append(pair);
            }
        }
    }
    QVariantMap report;
    report.insert(QStringLiteral("valid"), violatingPairs.isEmpty());
    report.insert(QStringLiteral("requiredSpacingMeters"), _minimumSpacingMeters);
    report.insert(QStringLiteral("minimumDistanceMeters"), minimumDistance);
    report.insert(QStringLiteral("pairCount"), pairCount);
    report.insert(QStringLiteral("violatingPairs"), violatingPairs);
    return report;
}

bool FormationPlanner::minimumSpacingValid(double requiredSpacingMeters) const
{
    if (requiredSpacingMeters < 0.0) {
        return minimumSpacingReport().value(QStringLiteral("valid")).toBool();
    }
    const QVariantList offsets = relativeOffsets();
    for (int i = 0; i < offsets.size(); ++i) {
        for (int j = i + 1; j < offsets.size(); ++j) {
            if (distanceBetween(offsets.at(i).toMap(), offsets.at(j).toMap()) + kEpsilon < requiredSpacingMeters) {
                return false;
            }
        }
    }
    return true;
}

QVariantList FormationPlanner::split(int groupCount) const
{
    QVariantList groups;
    if (groupCount <= 0 || _memberIds.isEmpty()) {
        return groups;
    }
    const int actualGroups = qMin(groupCount, _memberIds.size());
    const QVariantList offsets = relativeOffsets();
    const int baseSize = _memberIds.size() / actualGroups;
    const int remainder = _memberIds.size() % actualGroups;
    int cursor = 0;
    for (int groupIndex = 0; groupIndex < actualGroups; ++groupIndex) {
        const int size = baseSize + (groupIndex < remainder ? 1 : 0);
        QVariantList members;
        QVariantList groupOffsets;
        for (int i = 0; i < size; ++i) {
            members.append(_memberIds.at(cursor));
            if (cursor < offsets.size()) {
                groupOffsets.append(offsets.at(cursor));
            }
            ++cursor;
        }
        QVariantMap group;
        group.insert(QStringLiteral("groupIndex"), groupIndex);
        group.insert(QStringLiteral("members"), members);
        group.insert(QStringLiteral("offsets"), groupOffsets);
        groups.append(group);
    }
    return groups;
}

bool FormationPlanner::merge(const QVariantList& groups)
{
    QVariantList merged;
    QSet<QString> seen;
    for (const QVariant& group : groups) {
        QVariantList members;
        if (group.type() == QVariant::Map) {
            members = mapList(group.toMap(), QStringLiteral("members"));
        } else if (group.type() == QVariant::List) {
            members = group.toList();
        }
        for (const QVariant& member : members) {
            const QString key = valueKey(member);
            if (key.isEmpty() || seen.contains(key)) {
                continue;
            }
            seen.insert(key);
            merged.append(member);
        }
    }
    if (merged.isEmpty() && !groups.isEmpty()) {
        return false;
    }
    setMemberIds(merged);
    return true;
}

QVariantMap FormationPlanner::summary() const
{
    const QVariantMap spacing = minimumSpacingReport();
    QVariantMap reference;
    reference.insert(QStringLiteral("latitude"), _referenceLatitude);
    reference.insert(QStringLiteral("longitude"), _referenceLongitude);
    reference.insert(QStringLiteral("altitude"), _referenceAltitude);

    QVariantMap result;
    result.insert(QStringLiteral("formationType"), formationType());
    result.insert(QStringLiteral("memberCount"), _memberIds.size());
    result.insert(QStringLiteral("members"), _memberIds);
    result.insert(QStringLiteral("spacingMeters"), _spacingMeters);
    result.insert(QStringLiteral("scale"), _scale);
    result.insert(QStringLiteral("rotationDegrees"), _rotationDegrees);
    result.insert(QStringLiteral("reference"), reference);
    result.insert(QStringLiteral("minimumSpacing"), spacing);
    result.insert(QStringLiteral("minimumSpacingValid"), spacing.value(QStringLiteral("valid")));
    return result;
}

FormationPlanner::FormationType FormationPlanner::formationTypeFromString(const QString& type)
{
    const QString normalized = type.trimmed().toLower();
    if (normalized == QStringLiteral("column")) return Column;
    if (normalized == QStringLiteral("vee") || normalized == QStringLiteral("v")) return Vee;
    if (normalized == QStringLiteral("wedge")) return Wedge;
    if (normalized == QStringLiteral("circle") || normalized == QStringLiteral("ring")) return Circle;
    if (normalized == QStringLiteral("grid")) return Grid;
    if (normalized == QStringLiteral("custom")) return Custom;
    return Line;
}

QString FormationPlanner::formationTypeToString(FormationType type)
{
    switch (type) {
    case Column: return QStringLiteral("column");
    case Vee: return QStringLiteral("vee");
    case Wedge: return QStringLiteral("wedge");
    case Circle: return QStringLiteral("circle");
    case Grid: return QStringLiteral("grid");
    case Custom: return QStringLiteral("custom");
    case Line:
    default: return QStringLiteral("line");
    }
}

QVariantMap FormationPlanner::offsetMap(const QVariant& memberId, double x, double y, double z)
{
    QVariantMap offset;
    offset.insert(QStringLiteral("memberId"), memberId);
    offset.insert(QStringLiteral("x"), x);
    offset.insert(QStringLiteral("y"), y);
    offset.insert(QStringLiteral("z"), z);
    return offset;
}

QString FormationPlanner::valueKey(const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        return QString();
    }
    return value.toString().trimmed();
}

void FormationPlanner::emitPlanChanged()
{
    emit planChanged();
}
