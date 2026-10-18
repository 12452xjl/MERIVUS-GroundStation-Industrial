#include "FleetReplacementMatcher.h"

#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVector>

#include <cmath>
#include <functional>

namespace {

bool vehicleId(const QVariant& value, int* result)
{
    if (value.isNull() || !(value.type() == QVariant::Int || value.type() == QVariant::UInt
        || value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong
        || value.type() == QVariant::Double)) return false;
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !std::isfinite(number) || number < 1 || number > 255
        || std::floor(number) != number) return false;
    if (result) *result = static_cast<int>(number);
    return true;
}

} // namespace

QVariantMap FleetReplacementMatcher::match(const QVariantList& replacementSlots,
                                           const QVariantList& orderedCandidateVehicleIds)
{
    QVariantMap result{{QStringLiteral("inputValid"), false}, {QStringLiteral("complete"), false},
        {QStringLiteral("assignments"), QVariantList()},
        {QStringLiteral("unfilledSlotIds"), QStringList()},
        {QStringLiteral("inputIssues"), QStringList()},
        {QStringLiteral("recommendationOnly"), true}, {QStringLiteral("flightCommandReleased"), false}};
    const auto reject = [&](const QString& issue) {
        QVariantMap rejected = result;
        rejected.insert(QStringLiteral("inputIssues"), QStringList() << issue);
        return rejected;
    };
    if (replacementSlots.size() > 255 || orderedCandidateVehicleIds.size() > 255)
        return reject(QStringLiteral("replacement_matching_capacity_exceeded"));

    QVector<int> candidates;
    QHash<int, int> candidatePositions;
    for (const QVariant& raw : orderedCandidateVehicleIds) {
        int id = 0;
        if (!vehicleId(raw, &id) || candidatePositions.contains(id))
            return reject(QStringLiteral("invalid_or_duplicate_replacement_candidate"));
        candidatePositions.insert(id, candidates.size());
        candidates.append(id);
    }
    QStringList slotIds;
    QSet<QString> seenSlots;
    QVector<QVector<int>> graph;
    for (const QVariant& raw : replacementSlots) {
        if (raw.type() != QVariant::Map || raw.isNull())
            return reject(QStringLiteral("invalid_replacement_slot"));
        const QVariantMap slot = raw.toMap();
        const QVariant identity = slot.value(QStringLiteral("slotId"));
        const QVariant eligibility = slot.value(QStringLiteral("eligibleVehicleIds"));
        if (slot.size() != 2 || identity.type() != QVariant::String || identity.isNull()
            || identity.toString().isEmpty() || identity.toString().size() > 128
            || identity.toString().trimmed() != identity.toString()
            || QString::fromUtf8(identity.toString().toUtf8()) != identity.toString()
            || seenSlots.contains(identity.toString()) || eligibility.type() != QVariant::List
            || eligibility.isNull() || eligibility.toList().size() > candidates.size())
            return reject(QStringLiteral("invalid_or_duplicate_replacement_slot"));
        QSet<int> eligibleIds;
        for (const QVariant& entry : eligibility.toList()) {
            int id = 0;
            if (!vehicleId(entry, &id) || !candidatePositions.contains(id) || eligibleIds.contains(id))
                return reject(QStringLiteral("invalid_replacement_eligibility"));
            eligibleIds.insert(id);
        }
        QVector<int> edges;
        for (int index = 0; index < candidates.size(); ++index)
            if (eligibleIds.contains(candidates.at(index))) edges.append(index);
        graph.append(edges);
        seenSlots.insert(identity.toString());
        slotIds.append(identity.toString());
    }

    QVector<int> candidateOwner(candidates.size(), -1);
    QVector<int> slotCandidate(slotIds.size(), -1);
    // Augment, rather than first-fit: a general-purpose candidate can move to
    // another eligible slot so a specialist is not needlessly left unused.
    // A later failed slot never evicts a previously matched earlier slot.
    // At most 255 slots/candidates bound both recursion depth and edge visits.
    std::function<bool(int, QSet<int>&)> augment = [&](int slot, QSet<int>& visited) {
        for (const int candidate : graph.at(slot)) {
            if (visited.contains(candidate)) continue;
            visited.insert(candidate);
            const int owner = candidateOwner.at(candidate);
            if (owner < 0 || augment(owner, visited)) {
                candidateOwner[candidate] = slot;
                slotCandidate[slot] = candidate;
                return true;
            }
        }
        return false;
    };
    for (int slot = 0; slot < slotIds.size(); ++slot) {
        QSet<int> visited;
        augment(slot, visited);
    }
    QVariantList assignments;
    QStringList unfilled;
    for (int slot = 0; slot < slotIds.size(); ++slot) {
        const int candidate = slotCandidate.at(slot);
        if (candidate < 0) unfilled.append(slotIds.at(slot));
        else assignments.append(QVariantMap{{QStringLiteral("slotId"), slotIds.at(slot)},
            {QStringLiteral("vehicleId"), candidates.at(candidate)}});
    }
    result.insert(QStringLiteral("inputValid"), true);
    result.insert(QStringLiteral("complete"), unfilled.isEmpty());
    result.insert(QStringLiteral("assignments"), assignments);
    result.insert(QStringLiteral("unfilledSlotIds"), unfilled);
    return result;
}
