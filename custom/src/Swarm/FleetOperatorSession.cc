#include "FleetOperatorSession.h"
#include "FleetMissionStartupAuthority.h"
#include "FleetMissionStartupTransaction.h"
#include "FleetMissionGroundStartApproval.h"
#include "FleetMissionGroundStartupAuthority.h"
#include "FleetMissionObservationClose.h"
#include "FleetOccupancyRecoveryReview.h"
#include "FleetFirmwareCatalogReload.h"
#include "FleetCapabilityReview.h"
#include "FleetOptionalTelemetryConfiguration.h"
#include "FleetCameraStreamControl.h"
#include "FleetMissionDeploymentEpisode.h"

#include <QCoreApplication>
#include <QPointer>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>
#include <QUuid>
#include <cmath>
#include <limits>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <wincred.h>
#include <sddl.h>
#include <lm.h>
#include <objbase.h>
#include <wtsapi32.h>
#include <array>
#include <cstddef>
#include <cwchar>
#endif

namespace {
constexpr qint64 OperatorLifetimeMs = 5 * 60 * 1000;
constexpr qint64 ApproverLifetimeMs = 60 * 1000;
constexpr qint64 UploadLeaseMaximumMs = 4 * 60 * 1000;
const QString AuthenticationSource = QStringLiteral("windows-logon-local-role-groups");

class AuthenticationPromptStackGuard;
// GUI-thread-only state shared by every Session, including legacy objects and
// different deployment factories. Its owner is a native call stack, not a
// Session QObject that a credential/changed callback can destroy.
const AuthenticationPromptStackGuard* ActiveAuthenticationPrompt = nullptr;

class AuthenticationPromptStackGuard final
{
public:
    AuthenticationPromptStackGuard()
    {
        const QCoreApplication* application = QCoreApplication::instance();
        if (!application || QThread::currentThread() != application->thread()
            || ActiveAuthenticationPrompt) return;
        ActiveAuthenticationPrompt = this;
        _acquired = true;
    }
    AuthenticationPromptStackGuard(const AuthenticationPromptStackGuard&) = delete;
    AuthenticationPromptStackGuard& operator=(const AuthenticationPromptStackGuard&) = delete;
    AuthenticationPromptStackGuard(AuthenticationPromptStackGuard&&) = delete;
    AuthenticationPromptStackGuard& operator=(AuthenticationPromptStackGuard&&) = delete;
    ~AuthenticationPromptStackGuard()
    {
        // No QObject/application access: the original Session may already be
        // gone. A rejected nested guard can never unlock the outer call.
        if (_acquired && ActiveAuthenticationPrompt == this)
            ActiveAuthenticationPrompt = nullptr;
    }
    bool acquired() const { return _acquired; }

private:
    bool _acquired = false;
};

bool approverOutboundAllowed(const FleetOperatorSession* session, const QString& action)
{
    // Retained monitoring-close needs fresh account facts, not a new outbound
    // mission. All other approval purposes stay with the current actual group.
    return action == QStringLiteral("task.observation.close.approve")
        || FleetMissionDeploymentEpisode::outboundOwnerAllowed(session);
}

QString uuid()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool boundedText(const QString& value, int bytes)
{
    const QByteArray utf8 = value.toUtf8();
    if (value.isEmpty() || utf8.size() > bytes || QString::fromUtf8(utf8) != value) return false;
    for (QChar ch : value) {
        if (ch.isNull() || ch.category() == QChar::Other_Control
            || ch.category() == QChar::Other_Format) return false;
    }
    return true;
}

bool receiptNumber(const QVariant& value)
{
    constexpr qulonglong MaximumExactInteger = 9007199254740991ULL;
    // Qt 5's QML engine may return an integral JS Number as QVariant::Int.
    // Accept numeric storage representations, never bool/string conversions.
    switch (value.type()) {
    case QVariant::Int:
    case QVariant::LongLong:
        if (value.toLongLong() < 0
            || qulonglong(value.toLongLong()) > MaximumExactInteger) return false;
        break;
    case QVariant::UInt:
    case QVariant::ULongLong:
        if (value.toULongLong() > MaximumExactInteger) return false;
        break;
    case QVariant::Double:
        break;
    default:
        return false;
    }
    const double number = value.toDouble();
    return std::isfinite(number) && number >= 0 && number <= double(MaximumExactInteger)
        && std::floor(number) == number;
}

QVariantMap canonicalReceiptNumbers(QVariantMap receipt)
{
    // Only call after identityShape validated actual numeric types/range.
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"),
                               QStringLiteral("expiresAtMonotonicMs")})
        receipt[key] = receipt.value(key).toDouble();
    return receipt;
}

QVariantMap canonicalLeaseNumbers(QVariantMap receipt)
{
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("durationMs")}) receipt[key] = receipt.value(key).toDouble();
    return receipt;
}

QVariantMap canonicalStartAuthenticationNumbers(QVariantMap receipt)
{
    // Only after startAuthenticationShape has checked actual numeric types.
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("consumedAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("durationMs")}) receipt[key] = receipt.value(key).toDouble();
    return receipt;
}

bool startAuthenticationShape(const QVariantMap& receipt)
{
    if (receipt.size() != 16) return false;
    for (const QString& key : {QStringLiteral("approvalId"), QStringLiteral("approverAuthenticationId"),
        QStringLiteral("primarySessionId"), QStringLiteral("primaryActorId"), QStringLiteral("primaryRole"),
        QStringLiteral("approverActorId"), QStringLiteral("approverRole"), QStringLiteral("sessionEpoch"),
        QStringLiteral("sessionRevision"), QStringLiteral("action"), QStringLiteral("bindingHash")})
        if (receipt.value(key).type() != QVariant::String) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("consumedAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("durationMs")}) if (!receiptNumber(receipt.value(key))) return false;
    for (const QString& key : {QStringLiteral("approvalId"), QStringLiteral("approverAuthenticationId"),
        QStringLiteral("primarySessionId"), QStringLiteral("sessionEpoch")}) {
        const QString value = receipt.value(key).toString();
        if (QUuid(value).isNull() || QUuid(value).toString(QUuid::WithoutBraces) != value) return false;
    }
    const double consumed = receipt.value(QStringLiteral("consumedAtMonotonicMs")).toDouble();
    const double expires = receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const double windowStarted = receipt.value(QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs")).toDouble();
    const double duration = receipt.value(QStringLiteral("durationMs")).toDouble();
    return receipt.value(QStringLiteral("schemaVersion")).toDouble() == 1.0
        && duration >= 1 && duration <= ApproverLifetimeMs && expires - consumed == duration
        && consumed >= windowStarted && expires <= windowStarted + ApproverLifetimeMs
        && receipt.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && QRegularExpression(QStringLiteral("^[0-9a-f]{64}$"))
            .match(receipt.value(QStringLiteral("bindingHash")).toString()).hasMatch()
        && QRegularExpression(QStringLiteral("^[1-9][0-9]{0,19}$"))
            .match(receipt.value(QStringLiteral("sessionRevision")).toString()).hasMatch()
        && boundedText(receipt.value(QStringLiteral("primaryActorId")).toString(), 128)
        && boundedText(receipt.value(QStringLiteral("approverActorId")).toString(), 128);
}

QVariantMap canonicalStartupAuthenticationNumbers(QVariantMap receipt)
{
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("transferredAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("durationMs"),
        QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("originalAuthenticationExpiresAtMonotonicMs")}) receipt[key] = receipt.value(key).toDouble();
    return receipt;
}

bool startupAuthenticationShape(const QVariantMap& receipt)
{
    if (receipt.size() != 22) return false;
    for (const QString& key : {QStringLiteral("startupId"), QStringLiteral("authorityEpoch"),
        QStringLiteral("authenticationId"), QStringLiteral("sourceHash"), QStringLiteral("approvalId"),
        QStringLiteral("approverAuthenticationId"), QStringLiteral("primarySessionId"), QStringLiteral("sessionEpoch"),
        QStringLiteral("sessionRevision"), QStringLiteral("primaryActorId"), QStringLiteral("primaryRole"),
        QStringLiteral("approverActorId"), QStringLiteral("approverRole"), QStringLiteral("action"),
        QStringLiteral("bindingHash"), QStringLiteral("purpose")})
        if (receipt.value(key).type() != QVariant::String) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("transferredAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("durationMs"),
        QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("originalAuthenticationExpiresAtMonotonicMs")})
        if (!receiptNumber(receipt.value(key))) return false;
    for (const QString& key : {QStringLiteral("startupId"), QStringLiteral("authorityEpoch"),
        QStringLiteral("authenticationId"), QStringLiteral("approvalId"), QStringLiteral("approverAuthenticationId"),
        QStringLiteral("primarySessionId"), QStringLiteral("sessionEpoch")}) {
        const QString value = receipt.value(key).toString();
        if (QUuid(value).isNull() || QUuid(value).toString(QUuid::WithoutBraces) != value) return false;
    }
    const double transferred = receipt.value(QStringLiteral("transferredAtMonotonicMs")).toDouble();
    const double expires = receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const double duration = receipt.value(QStringLiteral("durationMs")).toDouble();
    const double started = receipt.value(QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs")).toDouble();
    return receipt.value(QStringLiteral("schemaVersion")).toDouble() == 1.0
        && receipt.value(QStringLiteral("purpose")).toString() == QStringLiteral("startup-authentication-basis-not-permit")
        && receipt.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && duration >= 1 && duration <= 30000 && expires - transferred == duration && transferred >= started
        && expires <= receipt.value(QStringLiteral("originalAuthenticationExpiresAtMonotonicMs")).toDouble()
        && QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(receipt.value(QStringLiteral("sourceHash")).toString()).hasMatch()
        && QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(receipt.value(QStringLiteral("bindingHash")).toString()).hasMatch()
        && QRegularExpression(QStringLiteral("^[1-9][0-9]{0,19}$")).match(receipt.value(QStringLiteral("sessionRevision")).toString()).hasMatch()
        && boundedText(receipt.value(QStringLiteral("primaryActorId")).toString(), 128)
        && boundedText(receipt.value(QStringLiteral("approverActorId")).toString(), 128);
}

bool startupTransactionAuthenticationShape(const QVariantMap& receipt)
{
    if (receipt.size() != 24 || receipt.value(QStringLiteral("transactionEpoch")).type() != QVariant::String
        || receipt.value(QStringLiteral("authorityAuthenticationId")).type() != QVariant::String
        || receipt.value(QStringLiteral("purpose")).type() != QVariant::String
        || receipt.value(QStringLiteral("purpose")).toString() != QStringLiteral("startup-transaction-authentication-basis-not-permit")) return false;
    for (const QString& key : {QStringLiteral("transactionEpoch"), QStringLiteral("authorityAuthenticationId")}) {
        const QString value = receipt.value(key).toString();
        if (QUuid(value).isNull() || QUuid(value).toString(QUuid::WithoutBraces) != value) return false;
    }
    QVariantMap basis = receipt;
    basis.remove(QStringLiteral("transactionEpoch")); basis.remove(QStringLiteral("authorityAuthenticationId"));
    basis[QStringLiteral("purpose")] = QStringLiteral("startup-authentication-basis-not-permit");
    return startupAuthenticationShape(basis);
}

QVariantMap canonicalGroundAuthenticationNumbers(QVariantMap receipt)
{
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("consumedAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("durationMs"), QStringLiteral("originalBudgetExpiresAtMonotonicMs")})
        receipt[key] = receipt.value(key).toDouble();
    return receipt;
}

bool groundAuthenticationShape(const QVariantMap& receipt)
{
    if (receipt.size() != 22) return false;
    for (const QString& key : {QStringLiteral("approvalId"), QStringLiteral("approvalEpoch"),
        QStringLiteral("reviewId"), QStringLiteral("reviewEpoch"), QStringLiteral("authenticationId"),
        QStringLiteral("purpose"), QStringLiteral("approverAuthenticationId"), QStringLiteral("primarySessionId"),
        QStringLiteral("primaryActorId"), QStringLiteral("primaryRole"), QStringLiteral("approverActorId"),
        QStringLiteral("approverRole"), QStringLiteral("sessionEpoch"), QStringLiteral("sessionRevision"),
        QStringLiteral("action"), QStringLiteral("bindingHash")})
        if (receipt.value(key).type() != QVariant::String) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("consumedAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("durationMs"), QStringLiteral("originalBudgetExpiresAtMonotonicMs")})
        if (!receiptNumber(receipt.value(key))) return false;
    for (const QString& key : {QStringLiteral("approvalId"), QStringLiteral("approvalEpoch"), QStringLiteral("reviewId"),
        QStringLiteral("reviewEpoch"), QStringLiteral("authenticationId"), QStringLiteral("approverAuthenticationId"),
        QStringLiteral("primarySessionId"), QStringLiteral("sessionEpoch")}) {
        const QString value = receipt.value(key).toString();
        if (QUuid(value).isNull() || QUuid(value).toString(QUuid::WithoutBraces) != value) return false;
    }
    const double consumed = receipt.value(QStringLiteral("consumedAtMonotonicMs")).toDouble();
    const double expires = receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const double started = receipt.value(QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs")).toDouble();
    const double budget = receipt.value(QStringLiteral("originalBudgetExpiresAtMonotonicMs")).toDouble();
    const double duration = receipt.value(QStringLiteral("durationMs")).toDouble();
    return receipt.value(QStringLiteral("schemaVersion")).toDouble() == 1.0 && duration >= 1 && duration <= 30000
        && expires - consumed == duration && consumed >= started && expires <= started + ApproverLifetimeMs
        && budget >= expires && budget - consumed <= 30000
        && receipt.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start.ground.reassess.approve")
        && receipt.value(QStringLiteral("purpose")).toString() == QStringLiteral("ground-reassess-conditional-authentication-not-permit")
        && QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(receipt.value(QStringLiteral("bindingHash")).toString()).hasMatch()
        && QRegularExpression(QStringLiteral("^[1-9][0-9]{0,19}$")).match(receipt.value(QStringLiteral("sessionRevision")).toString()).hasMatch()
        && boundedText(receipt.value(QStringLiteral("primaryActorId")).toString(), 128)
        && boundedText(receipt.value(QStringLiteral("approverActorId")).toString(), 128);
}

QVariantMap canonicalGroundStartupNumbers(QVariantMap receipt)
{
    receipt = canonicalGroundAuthenticationNumbers(receipt);
    for (const QString& key : {QStringLiteral("transferredAtMonotonicMs"), QStringLiteral("transferBudgetExpiresAtMonotonicMs")})
        receipt[key] = receipt.value(key).toDouble();
    return receipt;
}

bool groundStartupAuthenticationShape(const QVariantMap& receipt)
{
    if (receipt.size() != 28 || receipt.value(QStringLiteral("purpose")).type() != QVariant::String
        || receipt.value(QStringLiteral("purpose")).toString() != QStringLiteral("ground-startup-authentication-not-permit")) return false;
    for (const QString& key : {QStringLiteral("startupId"), QStringLiteral("authorityEpoch"), QStringLiteral("authorityAuthenticationId")}) {
        const QString value = receipt.value(key).toString();
        if (receipt.value(key).type() != QVariant::String || QUuid(value).isNull()
            || QUuid(value).toString(QUuid::WithoutBraces) != value) return false;
    }
    if (receipt.value(QStringLiteral("sourceHash")).type() != QVariant::String
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(receipt.value(QStringLiteral("sourceHash")).toString()).hasMatch()
        || !receiptNumber(receipt.value(QStringLiteral("transferredAtMonotonicMs")))
        || !receiptNumber(receipt.value(QStringLiteral("transferBudgetExpiresAtMonotonicMs")))) return false;
    QVariantMap basis = receipt;
    for (const QString& key : {QStringLiteral("startupId"), QStringLiteral("authorityEpoch"), QStringLiteral("sourceHash"),
        QStringLiteral("authorityAuthenticationId"), QStringLiteral("transferredAtMonotonicMs"), QStringLiteral("transferBudgetExpiresAtMonotonicMs")}) basis.remove(key);
    basis[QStringLiteral("purpose")] = QStringLiteral("ground-reassess-conditional-authentication-not-permit");
    // Reuse strict field-type/range checks only; the retained original 22-field
    // source below remains separate and cannot be restored from this map.
    if (!groundAuthenticationShape(basis)) return false;
    const double transferred = receipt.value(QStringLiteral("transferredAtMonotonicMs")).toDouble();
    const double budget = receipt.value(QStringLiteral("transferBudgetExpiresAtMonotonicMs")).toDouble();
    const double expires = receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    return transferred >= receipt.value(QStringLiteral("consumedAtMonotonicMs")).toDouble()
        && expires > transferred && budget >= expires && budget - transferred <= 30000;
}

bool groundStartupTransactionAuthenticationShape(const QVariantMap& receipt)
{
    if (receipt.size() != 30 || receipt.value(QStringLiteral("purpose")).type() != QVariant::String
        || receipt.value(QStringLiteral("purpose")).toString() != QStringLiteral("ground-startup-transaction-authentication-not-permit")) return false;
    for (const QString& key : {QStringLiteral("transactionEpoch"), QStringLiteral("transactionAuthenticationId")}) {
        const QString value = receipt.value(key).toString();
        if (receipt.value(key).type() != QVariant::String || QUuid(value).isNull()
            || QUuid(value).toString(QUuid::WithoutBraces) != value) return false;
    }
    QVariantMap basis = receipt;
    basis.remove(QStringLiteral("transactionEpoch")); basis.remove(QStringLiteral("transactionAuthenticationId"));
    basis[QStringLiteral("purpose")] = QStringLiteral("ground-startup-authentication-not-permit");
    return groundStartupAuthenticationShape(basis);
}

bool groundStartupOriginalBasisMatches(const QVariantMap& authorityBasis, const QVariantMap& approvalBasis)
{
    if (!groundStartupAuthenticationShape(authorityBasis) || !groundAuthenticationShape(approvalBasis)) return false;
    QVariantMap expected = canonicalGroundAuthenticationNumbers(approvalBasis);
    expected[QStringLiteral("expiresAtMonotonicMs")] = authorityBasis.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    expected[QStringLiteral("durationMs")] = authorityBasis.value(QStringLiteral("durationMs")).toDouble();
    expected[QStringLiteral("purpose")] = QStringLiteral("ground-startup-authentication-not-permit");
    for (const QString& key : {QStringLiteral("startupId"), QStringLiteral("authorityEpoch"), QStringLiteral("sourceHash"),
        QStringLiteral("authorityAuthenticationId"), QStringLiteral("transferredAtMonotonicMs"), QStringLiteral("transferBudgetExpiresAtMonotonicMs")})
        expected.insert(key, authorityBasis.value(key));
    return canonicalGroundStartupNumbers(authorityBasis) == expected
        && authorityBasis.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
            <= approvalBasis.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

QVariantMap startAuthenticationDenied(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("authenticationReceipt"), QVariantMap()},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool leaseShape(const QVariantMap& receipt)
{
    if (receipt.size() != 16) return false;
    for (const QString& key : {QStringLiteral("leaseId"), QStringLiteral("sessionEpoch"),
        QStringLiteral("sessionRevision"), QStringLiteral("primarySessionId"), QStringLiteral("primaryActorId"),
        QStringLiteral("primaryRole"), QStringLiteral("approverActorId"), QStringLiteral("approverRole"),
        QStringLiteral("approverAuthenticationId"), QStringLiteral("action"), QStringLiteral("bindingHash")})
        if (receipt.value(key).type() != QVariant::String) return false;
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"),
        QStringLiteral("expiresAtMonotonicMs"), QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"),
        QStringLiteral("durationMs")}) if (!receiptNumber(receipt.value(key))) return false;
    const double duration = receipt.value(QStringLiteral("durationMs")).toDouble();
    return receipt.value(QStringLiteral("schemaVersion")).toDouble() == 1.0
        && duration >= 1 && duration <= UploadLeaseMaximumMs
        && receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
            - receipt.value(QStringLiteral("issuedAtMonotonicMs")).toDouble() == duration
        && receipt.value(QStringLiteral("action")).toString() == QStringLiteral("mission.upload");
}

bool identityShape(const QVariantMap& receipt)
{
    if (receipt.size() != 11) return false;
    const QStringList strings {QStringLiteral("sessionId"), QStringLiteral("sessionEpoch"),
        QStringLiteral("sessionRevision"), QStringLiteral("actorId"), QStringLiteral("displayName"),
        QStringLiteral("role"), QStringLiteral("authenticationSource")};
    for (const QString& key : strings) {
        if (receipt.value(key).type() != QVariant::String) return false;
    }
    // QML may return Int for a copied integral Number. Strict numeric shape is
    // checked before canonicalization, so bool/string cannot become permission.
    for (const QString& key : {QStringLiteral("schemaVersion"), QStringLiteral("issuedAtMonotonicMs"),
                               QStringLiteral("expiresAtMonotonicMs")}) {
        if (!receiptNumber(receipt.value(key))) return false;
    }
    if (receipt.value(QStringLiteral("schemaVersion")).toDouble() != 1.0
        || receipt.value(QStringLiteral("authenticated")).type() != QVariant::Bool
        || !receipt.value(QStringLiteral("authenticated")).toBool()
        || receipt.value(QStringLiteral("authenticationSource")).toString() != AuthenticationSource
        || !boundedText(receipt.value(QStringLiteral("actorId")).toString(), 128)
        || !boundedText(receipt.value(QStringLiteral("displayName")).toString(), 256)) return false;
    const double issued = receipt.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double expires = receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    return expires > issued && expires - issued <= OperatorLifetimeMs
        && !QUuid(receipt.value(QStringLiteral("sessionId")).toString()).isNull()
        && !QUuid(receipt.value(QStringLiteral("sessionEpoch")).toString()).isNull()
        && QRegularExpression(QStringLiteral("^[1-9][0-9]{0,19}$"))
            .match(receipt.value(QStringLiteral("sessionRevision")).toString()).hasMatch();
}

bool approverRole(const QString& role)
{
    return role == QStringLiteral("safety_officer") || role == QStringLiteral("commander")
        || role == QStringLiteral("administrator");
}

bool approvalAction(const QString& action)
{
    // A closed purpose list, not a general command or caller-defined scope.
    return action == QStringLiteral("mission.upload") || action == QStringLiteral("mission.start")
        || action == QStringLiteral("mission.start.ground.reassess.approve")
        || action == QStringLiteral("fleet.occupancy.recover.approve")
        || action == QStringLiteral("firmware.catalog.reload.approve")
        || action == QStringLiteral("fleet.capability.review.approve")
        || action == QStringLiteral("fleet.performance.declaration.approve")
        || action == QStringLiteral("fleet.configuration.review.approve")
        || action == QStringLiteral("fleet.telemetry.interval.configure.approve")
        || action == QStringLiteral("fleet.video.stream.control.approve")
        || action == QStringLiteral("task.observation.close.approve");
}

bool summaryText(const QString& value)
{
    const QByteArray utf8 = value.toUtf8();
    if (value.trimmed().isEmpty() || value.size() > 600 || utf8.size() > 1800
        || QString::fromUtf8(utf8) != value) return false;
    for (QChar ch : value) {
        if (ch.isNull() || ch.category() == QChar::Other_Format
            || (ch.category() == QChar::Other_Control && ch != QChar('\n'))) return false;
    }
    return true;
}

#ifdef Q_OS_WIN
// No secret is ever converted into QString/QVariant or retained by this object.
template<size_t N> struct SecretWideBuffer {
    SecretWideBuffer() = default;
    SecretWideBuffer(const SecretWideBuffer&) = delete;
    SecretWideBuffer& operator=(const SecretWideBuffer&) = delete;
    std::array<WCHAR, N> value {};
    ~SecretWideBuffer() { SecureZeroMemory(value.data(), sizeof(value)); }
    WCHAR* data() { return value.data(); }
    DWORD capacity() const { return static_cast<DWORD>(N); }
};

template<size_t N> struct SecretByteBuffer {
    SecretByteBuffer() = default;
    SecretByteBuffer(const SecretByteBuffer&) = delete;
    SecretByteBuffer& operator=(const SecretByteBuffer&) = delete;
    alignas(std::max_align_t) std::array<unsigned char, N> value {};
    ~SecretByteBuffer() { SecureZeroMemory(value.data(), value.size()); }
    unsigned char* data() { return value.data(); }
};

struct CredentialBlob {
    CredentialBlob() = default;
    CredentialBlob(const CredentialBlob&) = delete;
    CredentialBlob& operator=(const CredentialBlob&) = delete;
    LPVOID value = nullptr;
    ULONG size = 0;
    ~CredentialBlob() {
        if (value) {
            SecureZeroMemory(value, size);
            CoTaskMemFree(value);
        }
    }
};

struct TokenHandle {
    TokenHandle() = default;
    TokenHandle(const TokenHandle&) = delete;
    TokenHandle& operator=(const TokenHandle&) = delete;
    HANDLE value = nullptr;
    ~TokenHandle() { if (value) CloseHandle(value); }
};

struct LocalBuffer {
    LocalBuffer() = default;
    LocalBuffer(const LocalBuffer&) = delete;
    LocalBuffer& operator=(const LocalBuffer&) = delete;
    HLOCAL value = nullptr;
    ~LocalBuffer() { if (value) LocalFree(value); }
};

struct NetBuffer {
    NetBuffer() = default;
    NetBuffer(const NetBuffer&) = delete;
    NetBuffer& operator=(const NetBuffer&) = delete;
    LPBYTE value = nullptr;
    ~NetBuffer() { if (value) NetApiBufferFree(value); }
};

struct RoleGroup { const WCHAR* group; const char* role; };
const RoleGroup RoleGroups[] {
    {L"MERIVUS_Fleet_Observer", "observer"},
    {L"MERIVUS_Fleet_Planner", "planner"},
    {L"MERIVUS_Fleet_Operator", "operator"},
    {L"MERIVUS_Fleet_SafetyOfficer", "safety_officer"},
    {L"MERIVUS_Fleet_Maintenance", "maintenance"},
    {L"MERIVUS_Fleet_Commander", "commander"},
    {L"MERIVUS_Fleet_Administrator", "administrator"}
};

QString roleForSid(PSID actorSid)
{
    if (!actorSid || !IsValidSid(actorSid)) return QString();
    QString selected;
    for (const RoleGroup& entry : RoleGroups) {
        NetBuffer members;
        DWORD read = 0;
        DWORD total = 0;
        // No resumable enumeration handle is retained. Reject incomplete or
        // oversized groups rather than trusting a prefix or allocating all SAM.
        const NET_API_STATUS status = NetLocalGroupGetMembers(nullptr, entry.group, 0,
            &members.value, 256 * 1024, &read, &total, nullptr);
        if (status == ERROR_NO_SUCH_ALIAS || status == NERR_GroupNotFound) continue;
        if (status != NERR_Success || read != total || read > 4096
            || (read != 0 && !members.value)) return QString();
        const auto entries = reinterpret_cast<const LOCALGROUP_MEMBERS_INFO_0*>(members.value);
        bool belongs = false;
        for (DWORD index = 0; index < read; ++index) {
            if (!entries[index].lgrmi0_sid || !IsValidSid(entries[index].lgrmi0_sid)) return QString();
            // No nested group expansion, token-groups shortcut, username match,
            // default observer grant, or implicit Windows administrator grant.
            if (EqualSid(actorSid, entries[index].lgrmi0_sid)) belongs = true;
        }
        if (belongs) {
            if (!selected.isEmpty()) return QString(); // Ambiguous policy fails closed.
            selected = QString::fromLatin1(entry.role);
        }
    }
    return selected;
}

QString currentRole(const QString& actorId)
{
    LocalBuffer sid;
    PSID nativeSid = nullptr;
    if (!ConvertStringSidToSidW(reinterpret_cast<LPCWSTR>(actorId.utf16()), &nativeSid)) return QString();
    sid.value = nativeSid;
    return roleForSid(nativeSid);
}

struct NativeIdentity {
    QString actorId;
    QString displayName;
    QString role;
    QString error;
};

NativeIdentity promptIdentity(const QString& message)
{
    NativeIdentity result;
    if (message.isEmpty() || message.size() > 1024 || message.size() > CREDUI_MAX_MESSAGE_LENGTH) {
        result.error = QStringLiteral("credential-prompt-message-unsupported");
        return result;
    }
    CREDUI_INFOW ui {};
    ui.cbSize = sizeof(ui);
    ui.hwndParent = GetActiveWindow();
    ui.pszCaptionText = L"MERIVUS Fleet - Windows identity";
    ui.pszMessageText = reinterpret_cast<LPCWSTR>(message.utf16());
    CredentialBlob blob;
    ULONG package = 0;
    // Secure desktop, no saved credential checkbox, no prefilled secret, no
    // administrator enumeration or generic plaintext-provider downgrade.
    const DWORD status = CredUIPromptForWindowsCredentialsW(&ui, 0, &package,
        nullptr, 0, &blob.value, &blob.size, nullptr, CREDUIWIN_SECURE_PROMPT);
    if (status != ERROR_SUCCESS) {
        result.error = status == ERROR_CANCELLED ? QStringLiteral("credential-prompt-cancelled")
                                                : QStringLiteral("credential-prompt-unavailable");
        return result;
    }
    if (!blob.value || blob.size == 0 || blob.size > 1024 * 1024) {
        result.error = QStringLiteral("credential-buffer-unsupported");
        return result;
    }
    SecretWideBuffer<513> user;
    SecretWideBuffer<257> domain;
    SecretWideBuffer<4097> password;
    DWORD userLength = user.capacity();
    DWORD domainLength = domain.capacity();
    DWORD passwordLength = password.capacity();
    if (!CredUnPackAuthenticationBufferW(CRED_PACK_PROTECTED_CREDENTIALS, blob.value, blob.size,
        user.data(), &userLength, domain.data(), &domainLength, password.data(), &passwordLength)
        || userLength > user.capacity() || domainLength > domain.capacity()
        || passwordLength > password.capacity() || user.data()[0] == L'\0'
        || password.data()[0] == L'\0') {
        result.error = QStringLiteral("credential-provider-or-format-unsupported");
        return result;
    }
    SecretWideBuffer<513> parsedUser;
    SecretWideBuffer<257> parsedDomain;
    LPCWSTR logonUser = user.data();
    LPCWSTR logonDomain = domain.data()[0] ? domain.data() : L".";
    if (std::wcschr(user.data(), L'\\') || std::wcschr(user.data(), L'@')) {
        if (CredUIParseUserNameW(user.data(), parsedUser.data(), parsedUser.capacity(),
                                parsedDomain.data(), parsedDomain.capacity()) != NO_ERROR
            || parsedUser.data()[0] == L'\0'
            || (domain.data()[0] && parsedDomain.data()[0]
                && _wcsicmp(domain.data(), parsedDomain.data()) != 0)) {
            result.error = QStringLiteral("credential-account-format-ambiguous");
            return result;
        }
        logonUser = parsedUser.data();
        // UPN retains its complete user@domain and LogonUser requires NULL.
        logonDomain = std::wcschr(parsedUser.data(), L'@') ? nullptr : parsedDomain.data();
        if (logonDomain && logonDomain[0] == L'\0') {
            result.error = QStringLiteral("credential-account-domain-missing");
            return result;
        }
    }
    TokenHandle token;
    if (!LogonUserW(logonUser, logonDomain, password.data(), LOGON32_LOGON_INTERACTIVE,
                    LOGON32_PROVIDER_DEFAULT, &token.value) || !token.value) {
        result.error = QStringLiteral("windows-logon-rejected");
        return result;
    }
    // Clear the plaintext and packed buffers immediately after authentication,
    // before account lookup or group enumeration; RAII still wipes all exits.
    SecureZeroMemory(password.data(), sizeof(password.value));
    SecureZeroMemory(blob.value, blob.size);
    SecureZeroMemory(user.data(), sizeof(user.value));
    SecureZeroMemory(domain.data(), sizeof(domain.value));
    SecureZeroMemory(parsedUser.data(), sizeof(parsedUser.value));
    SecureZeroMemory(parsedDomain.data(), sizeof(parsedDomain.value));
    SecretByteBuffer<4096> tokenInfo;
    DWORD required = 0;
    if (!GetTokenInformation(token.value, TokenUser, tokenInfo.data(), sizeof(tokenInfo.value), &required)
        || required < sizeof(TOKEN_USER) || required > sizeof(tokenInfo.value)) {
        result.error = QStringLiteral("authenticated-token-user-unavailable");
        return result;
    }
    auto tokenUser = reinterpret_cast<TOKEN_USER*>(tokenInfo.data());
    if (!tokenUser->User.Sid || !IsValidSid(tokenUser->User.Sid)) {
        result.error = QStringLiteral("authenticated-token-sid-invalid");
        return result;
    }
    LPWSTR sidText = nullptr;
    if (!ConvertSidToStringSidW(tokenUser->User.Sid, &sidText) || !sidText) {
        result.error = QStringLiteral("authenticated-token-sid-unavailable");
        return result;
    }
    LocalBuffer sidTextOwner;
    sidTextOwner.value = sidText;
    result.actorId = QString::fromWCharArray(sidText);
    if (!boundedText(result.actorId, 128)) {
        result.error = QStringLiteral("authenticated-token-sid-unsupported");
        return result;
    }
    result.role = roleForSid(tokenUser->User.Sid);
    if (result.role.isEmpty()) {
        result.error = QStringLiteral("local-role-membership-unavailable-or-ambiguous");
        return result;
    }
    SecretWideBuffer<513> name;
    SecretWideBuffer<257> nameDomain;
    DWORD nameLength = name.capacity();
    DWORD nameDomainLength = nameDomain.capacity();
    SID_NAME_USE use = SidTypeUnknown;
    result.displayName = result.actorId; // Display name never authorizes an account.
    if (LookupAccountSidW(nullptr, tokenUser->User.Sid, name.data(), &nameLength,
                         nameDomain.data(), &nameDomainLength, &use)
        && nameLength < name.capacity() && nameDomainLength < nameDomain.capacity()
        && use == SidTypeUser) {
        const QString display = (nameDomain.data()[0]
            ? QString::fromWCharArray(nameDomain.data()) + QStringLiteral("\\") : QString())
            + QString::fromWCharArray(name.data());
        if (boundedText(display, 256)) result.displayName = display;
    }
    return result;
}
#else
QString currentRole(const QString&) { return QString(); }
#endif
}

#ifdef Q_OS_WIN
// Owned, invisible, message-only HWND: WTS targets this registered window
// directly. Unlock/connect notices never recreate credentials or a lease.
class FleetWindowsSessionWatch
{
public:
    explicit FleetWindowsSessionWatch(FleetOperatorSession* owner) : _owner(owner)
    {
        // Query the real kernel version, not Qt's DEBUG environment override or
        // manifest-dependent GetVersionEx (Windows 7 has inverted WTS flags).
        using RtlVersionFunction = LONG (WINAPI*)(OSVERSIONINFOW*);
        const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        const auto rtlVersion = ntdll ? reinterpret_cast<RtlVersionFunction>(
            GetProcAddress(ntdll, "RtlGetVersion")) : nullptr;
        OSVERSIONINFOW version {};
        version.dwOSVersionInfoSize = sizeof(version);
        if (!rtlVersion || rtlVersion(&version) != 0 || version.dwMajorVersion < 10
            || !ProcessIdToSessionId(GetCurrentProcessId(), &_sessionId) || _sessionId == 0) return;
        _instance = GetModuleHandleW(nullptr);
        _className = QStringLiteral("MERIVUSFleetWts_") + uuid();
        WNDCLASSEXW windowClass {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = &FleetWindowsSessionWatch::windowProc;
        windowClass.hInstance = _instance;
        windowClass.lpszClassName = reinterpret_cast<LPCWSTR>(_className.utf16());
        _classAtom = RegisterClassExW(&windowClass);
        if (!_classAtom) return;
        _window = CreateWindowExW(0, windowClass.lpszClassName, L"", 0, 0, 0, 0, 0,
                                 HWND_MESSAGE, nullptr, _instance, this);
        if (!_window) return;
        _registered = WTSRegisterSessionNotification(_window, NOTIFY_FOR_THIS_SESSION) != FALSE;
    }

    ~FleetWindowsSessionWatch()
    {
        // Stop callbacks before native cleanup; destruction can occur from a
        // revocation signal emitted by this window's own message callback.
        _owner.clear();
        if (_window) {
            SetWindowLongPtrW(_window, GWLP_USERDATA, 0);
            if (_registered) WTSUnRegisterSessionNotification(_window);
            DestroyWindow(_window);
        }
        if (_classAtom) UnregisterClassW(reinterpret_cast<LPCWSTR>(_className.utf16()), _instance);
    }

    bool registered() const { return _registered && _window; }

    bool unlocked() const
    {
        // Freeze every member before Windows calls that can dispatch messages.
        // This helper/owner may be deleted by an independent WTS revocation.
        const HWND window = _window;
        const DWORD sessionId = _sessionId;
        const bool registered = _registered;
        if (!registered || !window || !IsWindow(window)) return false;
        MSG pending {};
        if (PeekMessageW(&pending, window, WM_WTSSESSION_CHANGE, WM_WTSSESSION_CHANGE, PM_NOREMOVE))
            return false; // Do not authorize before queued session changes are dispatched.
        LPWSTR buffer = nullptr;
        DWORD bytes = 0;
        const BOOL received = WTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId,
                                                          WTSSessionInfoEx, &buffer, &bytes);
        bool valid = false;
        if (received && buffer && bytes >= sizeof(WTSINFOEXW) && bytes <= 64 * 1024) {
            const auto info = reinterpret_cast<const WTSINFOEXW*>(buffer);
            valid = info->Level == 1 && info->Data.WTSInfoExLevel1.SessionId == sessionId
                && info->Data.WTSInfoExLevel1.SessionState == WTSActive
                && info->Data.WTSInfoExLevel1.SessionFlags == WTS_SESSIONSTATE_UNLOCK;
        }
        if (buffer) {
            SecureZeroMemory(buffer, bytes);
            WTSFreeMemory(buffer);
        }
        return valid && IsWindow(window);
    }

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_NCCREATE) {
            const auto create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            SetLastError(ERROR_SUCCESS);
            if (!create || (!SetWindowLongPtrW(window, GWLP_USERDATA,
                    reinterpret_cast<LONG_PTR>(create->lpCreateParams)) && GetLastError() != ERROR_SUCCESS))
                return FALSE;
        }
        auto watch = reinterpret_cast<FleetWindowsSessionWatch*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        QPointer<FleetOperatorSession> owner;
        QString reason;
        if (watch && message == WM_WTSSESSION_CHANGE && DWORD(lParam) == watch->_sessionId) {
            owner = watch->_owner;
            if (wParam == WTS_SESSION_LOCK) reason = QStringLiteral("windows-workstation-locked");
            else if (wParam == WTS_SESSION_LOGOFF) reason = QStringLiteral("windows-session-logged-off");
            else if (wParam == WTS_CONSOLE_DISCONNECT || wParam == WTS_REMOTE_DISCONNECT)
                reason = QStringLiteral("windows-session-disconnected");
            else if (wParam == WTS_SESSION_TERMINATE) reason = QStringLiteral("windows-session-terminated");
        } else if (watch && message == WM_NCDESTROY) {
            owner = watch->_owner;
            if (watch->_registered) WTSUnRegisterSessionNotification(window);
            watch->_registered = false;
            watch->_window = nullptr;
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            reason = QStringLiteral("windows-session-monitor-destroyed");
        }
        // Finish native default handling before notifying Qt. No watch/window
        // dereference occurs after a signal that can destroy this object.
        const LRESULT result = DefWindowProcW(window, message, wParam, lParam);
        if (owner && !reason.isEmpty()) owner->workstationRevoked(reason);
        return result;
    }

    QPointer<FleetOperatorSession> _owner;
    DWORD _sessionId = 0;
    HINSTANCE _instance = nullptr;
    QString _className;
    ATOM _classAtom = 0;
    HWND _window = nullptr;
    bool _registered = false;
};
#endif

FleetOperatorSession::FleetOperatorSession(QObject* parent)
    : QObject(parent), _sessionEpoch(uuid())
{
    _clock.start();
#ifdef Q_OS_WIN
    if (QCoreApplication::instance() && QThread::currentThread() == QCoreApplication::instance()->thread())
        _windowsSessionWatch.reset(new FleetWindowsSessionWatch(this));
#endif
    _expiryTimer.setInterval(1000);
    connect(&_expiryTimer, &QTimer::timeout, this, &FleetOperatorSession::expireReceipts);
    _expiryTimer.start();
}

FleetOperatorSession::~FleetOperatorSession() = default;

bool FleetOperatorSession::ownerThread() const
{
    return QCoreApplication::instance() && QElapsedTimer::isMonotonic() && _clock.isValid()
        && _clock.elapsed() >= 0 && _clock.elapsed() < 9007199254740991LL - OperatorLifetimeMs
        && QThread::currentThread() == thread()
        && thread() == QCoreApplication::instance()->thread();
}

bool FleetOperatorSession::authenticationPromptInProgress()
{
    const QCoreApplication* application = QCoreApplication::instance();
    if (!application || QThread::currentThread() != application->thread()) return true;
    return ActiveAuthenticationPrompt != nullptr;
}

bool FleetOperatorSession::advanceSession()
{
    _identityReceipt.clear();
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    clearUploadIdentityLease();
    clearStartApprovalAuthentication();
    clearGroundStartApprovalAuthentication();
    clearGroundStartupAuthentication();
    clearGroundStartupTransactionAuthentication();
    clearStartupAuthentication();
    clearStartupTransactionAuthentication();
    clearTelemetryConfigurationAuthentication();
    clearCameraStreamControlAuthentication();
    _sessionEpoch = uuid();
    if (_sessionRevision == std::numeric_limits<quint64>::max()) {
        _revisionExhausted = true;
        return false;
    }
    ++_sessionRevision;
    return !_revisionExhausted;
}

QVariantMap FleetOperatorSession::denied(const QString& reason)
{
    return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), reason},
        {QStringLiteral("identityReceipt"), QVariantMap()},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::identityCurrent(const QVariantMap& receipt, bool allowPrompt) const
{
    const QVariantMap identity = receipt; // Member input can be cleared by an OS callback.
    if (!ownerThread() || _revisionExhausted || (_promptActive && !allowPrompt)
        || !identityShape(identity) || canonicalReceiptNumbers(identity) != _identityReceipt
        || identity.value(QStringLiteral("sessionEpoch")).toString() != _sessionEpoch
        || identity.value(QStringLiteral("sessionRevision")).toString() != QString::number(_sessionRevision)
        || _clock.elapsed() >= identity.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) return false;
    QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QVariantMap frozen = _identityReceipt;
    if (!workstationCurrent() || !self || _identityReceipt != frozen) return false;
    const QString role = currentRole(identity.value(QStringLiteral("actorId")).toString());
    if (!self || !workstationCurrent() || !self) return false;
    return self && _identityReceipt == frozen && !role.isEmpty()
        && role == identity.value(QStringLiteral("role")).toString()
        && _clock.elapsed() < identity.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::workstationCurrent() const
{
#ifdef Q_OS_WIN
    if (!ownerThread() || !_windowsSessionWatch) return false;
    QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const bool unlocked = _windowsSessionWatch->unlocked();
    return self && unlocked;
#else
    return false;
#endif
}

void FleetOperatorSession::workstationRevoked(const QString& reason)
{
    if (!ownerThread()) return;
    advanceSession();
    _state = QStringLiteral("revoked");
    _lastError = reason;
    emit changed();
}

QVariantMap FleetOperatorSession::identitySnapshot() const
{
    return identityCurrent(_identityReceipt, true) ? _identityReceipt : QVariantMap();
}

bool FleetOperatorSession::identitySnapshotCurrent(const QVariantMap& receipt) const
{
    return identityCurrent(receipt, false);
}

QVariantMap FleetOperatorSession::current() const
{
    QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QVariantMap identity = identitySnapshot();
    if (!self) return {{QStringLiteral("authenticated"), false}, {QStringLiteral("identityReceipt"), QVariantMap()},
        {QStringLiteral("uploadIdentityLease"), QVariantMap()}, {QStringLiteral("flightCommandReleased"), false}};
#ifdef Q_OS_WIN
    const bool platformSupported = true;
    const bool monitorAvailable = _windowsSessionWatch && _windowsSessionWatch->registered();
#else
    const bool platformSupported = false;
    const bool monitorAvailable = false;
#endif
    return {{QStringLiteral("platformSupported"), platformSupported},
        {QStringLiteral("workstationMonitorAvailable"), monitorAvailable},
        {QStringLiteral("busy"), _promptActive}, {QStringLiteral("authenticated"), !identity.isEmpty()},
        {QStringLiteral("state"), _promptActive ? QStringLiteral("authenticating")
            : (!_identityReceipt.isEmpty() && identity.isEmpty() ? QStringLiteral("identity-not-current") : _state)},
        {QStringLiteral("reason"), _lastError}, {QStringLiteral("identityReceipt"), identity},
        {QStringLiteral("uploadIdentityLease"), _uploadIdentityLease},
        {QStringLiteral("authenticationSource"), AuthenticationSource},
        {QStringLiteral("flightCommandReleased"), false}};
}

QVariantMap FleetOperatorSession::authenticateOperator()
{
    if (!ownerThread()) return denied(QStringLiteral("authentication-owner-thread-or-busy"));
    // First local after the pure owner-thread check, hence last local to leave:
    // the lock includes OS queries, modal dispatch, all changed callbacks and
    // final identity validation, even if any of them destroys this Session.
    const AuthenticationPromptStackGuard promptStack;
    if (!promptStack.acquired() || _promptActive)
        return denied(QStringLiteral("authentication-owner-thread-or-busy"));
#ifndef Q_OS_WIN
    return denied(QStringLiteral("windows-local-authentication-not-supported"));
#else
    QPointer<FleetOperatorSession> entrySelf(this);
    if (!workstationCurrent() || !entrySelf) return {{QStringLiteral("ok"), false},
        {QStringLiteral("reason"), QStringLiteral("windows-workstation-not-active-unlocked")}};
    if (!advanceSession()) return denied(QStringLiteral("session-revision-exhausted"));
    const QString epoch = _sessionEpoch;
    const quint64 revision = _sessionRevision;
    const qint64 issued = _clock.elapsed();
    _promptActive = true;
    _state = QStringLiteral("signed-out");
    _lastError.clear();
    QPointer<FleetOperatorSession> self(this);
    emit changed();
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("session-destroyed")}};
    if (!workstationCurrent() || !self || _sessionEpoch != epoch || _sessionRevision != revision) {
        if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("session-destroyed")}};
        _promptActive = false;
        const QVariantMap failure = denied(QStringLiteral("authentication-source-changed"));
        emit changed();
        return failure;
    }
    const NativeIdentity native = promptIdentity(QStringLiteral(
        "Authenticate the operator for MERIVUS Fleet. Use an account directly assigned to exactly one MERIVUS_Fleet role group. No flight command will be issued."));
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("session-destroyed")}};
    _promptActive = false;
    QString error = native.error;
    if (!workstationCurrent() || !self || _sessionEpoch != epoch || _sessionRevision != revision) {
        if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("session-destroyed")}};
        error = QStringLiteral("authentication-source-changed");
    }
    else if (_clock.elapsed() >= issued + OperatorLifetimeMs) error = QStringLiteral("authentication-expired-during-prompt");
    else if (error.isEmpty() && currentRole(native.actorId) != native.role) error = QStringLiteral("local-role-changed-during-prompt");
    if (!self) return denied(QStringLiteral("session-destroyed"));
    if (!workstationCurrent()) error = QStringLiteral("windows-workstation-not-active-unlocked");
    if (!self) return denied(QStringLiteral("session-destroyed"));
    if (_sessionEpoch != epoch || _sessionRevision != revision || _clock.elapsed() >= issued + OperatorLifetimeMs)
        error = QStringLiteral("authentication-source-changed-or-expired");
    if (!error.isEmpty()) {
        _lastError = error;
        const QVariantMap failure = denied(error);
        emit changed();
        return failure;
    }
    _identityReceipt = {{QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("sessionId"), uuid()},
        {QStringLiteral("sessionEpoch"), epoch}, {QStringLiteral("sessionRevision"), QString::number(revision)},
        {QStringLiteral("actorId"), native.actorId}, {QStringLiteral("displayName"), native.displayName},
        {QStringLiteral("role"), native.role}, {QStringLiteral("authenticationSource"), AuthenticationSource},
        {QStringLiteral("authenticated"), true}, {QStringLiteral("issuedAtMonotonicMs"), double(issued)},
        {QStringLiteral("expiresAtMonotonicMs"), double(issued + OperatorLifetimeMs)}};
    _state = QStringLiteral("authenticated");
    const QVariantMap receipt = _identityReceipt;
    emit changed();
    if (!self || !identitySnapshotCurrent(receipt))
        return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("authentication-source-changed")}};
    return {{QStringLiteral("ok"), true}, {QStringLiteral("identityReceipt"), receipt},
        {QStringLiteral("flightCommandReleased"), false}};
#endif
}

void FleetOperatorSession::logout()
{
    if (!ownerThread()) return;
    advanceSession();
    _state = QStringLiteral("signed-out");
    _lastError.clear();
    emit changed();
}

QVariantMap FleetOperatorSession::authenticateApprover(const QString& requestedAction, const QString& requestedBindingHash,
                                                      const QVariantMap& requestedApplicantIdentity, const QString& requestedOperationSummary)
{
    if (!ownerThread()) return denied(QStringLiteral("approver-request-invalid"));
    const AuthenticationPromptStackGuard promptStack;
    if (!promptStack.acquired()) return denied(QStringLiteral("authentication-prompt-stack-busy"));
    // A native caller may pass its own member fields, then be destroyed by a
    // WTS callback or the credential modal while this Session survives.
    const QString action = requestedAction;
    const QString bindingHash = requestedBindingHash;
    const QVariantMap applicantIdentity = requestedApplicantIdentity;
    const QString operationSummary = requestedOperationSummary;
    QPointer<FleetOperatorSession> entrySelf(this);
    if (_promptActive || !approvalAction(action) || !approverOutboundAllowed(this, action)
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || !summaryText(operationSummary)
        || !identitySnapshotCurrent(applicantIdentity) || !entrySelf || !approverOutboundAllowed(this, action))
        return denied(QStringLiteral("approver-request-invalid"));
#ifndef Q_OS_WIN
    return denied(QStringLiteral("windows-local-authentication-not-supported"));
#else
    const qint64 issued = _clock.elapsed();
    const qint64 expires = qMin(issued + ApproverLifetimeMs,
        qint64(applicantIdentity.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()));
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    _promptActive = true;
    QPointer<FleetOperatorSession> self(this);
    emit changed();
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("session-destroyed")}};
    const bool applicantStillCurrent = approverOutboundAllowed(this, action) && identityCurrent(applicantIdentity, true);
    if (!self) return denied(QStringLiteral("session-destroyed"));
    if (!applicantStillCurrent || !approverOutboundAllowed(this, action)) {
        _promptActive = false;
        const QVariantMap failure = denied(QStringLiteral("applicant-source-changed"));
        emit changed();
        return failure;
    }
    const QString prompt = action == QStringLiteral("fleet.occupancy.recover.approve") ? QStringLiteral(
        "Approve fleet.occupancy.recover ONLY: review and retire this instance's selected restart/historical LOCAL review holds. NO STOP, DISARM, ARM or START. This does NOT verify remote freedom, aircraft safety, physical disposition or completion.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT approver account.")
        : action == QStringLiteral("firmware.catalog.reload.approve") ? QStringLiteral(
        "Approve firmware.catalog.reload ONLY: replace this instance's LOCAL engineering declaration catalog with the reviewed file content. Existing mission reviews are invalidated. NO firmware flashing, parameter write, upload, ARM or START; NOT firmware authentication or signature verification.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT safety officer/commander/MERIVUS administrator account.")
        : action == QStringLiteral("fleet.capability.review.approve") ? QStringLiteral(
        "Approve fleet.capability.review ONLY: approve or revoke the frozen LOCAL engineering capability declaration for one registry record. NOT hardware identity authentication, measured-performance certification or configuration-baseline approval. NO parameter write, upload, ARM, START or flight permission.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT safety officer/commander/MERIVUS administrator account.")
        : action == QStringLiteral("fleet.performance.declaration.approve") ? QStringLiteral(
        "Approve fleet.performance.declaration ONLY: adopt the frozen proposed LOCAL engineering speed/endurance numbers and capabilities for one registry record against its complete matching configuration baseline. This is NOT measured-performance certification, hardware authentication or configuration-baseline approval; other capability/baseline receipts cannot approve this change. NO parameter write, upload, ARM, START or flight permission.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT safety officer/commander/MERIVUS administrator account.")
        : action == QStringLiteral("fleet.configuration.review.approve") ? QStringLiteral(
        "Approve fleet.configuration.review ONLY: approve or revoke the frozen LOCAL ground-station configuration BASELINE for one registry record. This is NOT capability adoption, physical-modification detection, hardware authentication or performance certification. NO onboard/parameter write, upload, ARM, START or flight permission.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT safety officer/commander/MERIVUS administrator account.")
        : action == QStringLiteral("fleet.telemetry.interval.configure.approve") ? QStringLiteral(
        "Approve optional telemetry intervals ONLY for 62/74/83 on the frozen ground vehicle. This MAY SEND SET_MESSAGE_INTERVAL followed by readbacks. Positive reviewed intervals only; no automatic retry, restore or rollback. ACK/reported values do NOT prove a transmit rate or response attribution. NO parameter write, upload, ARM, START or flight permission.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT safety officer/commander/MERIVUS administrator account.")
        : action == QStringLiteral("fleet.video.stream.control.approve") ? QStringLiteral(
        "Approve ONE reviewed VIDEO STREAM STOP or START only. This MAY SEND a camera stream command and status readback, including while the aircraft flies. STOP may remove operator video. No automatic retry, restore or rollback. ACK/reported state does NOT prove actual video delivery/stop, bandwidth or response attribution. NO parameter write, upload, ARM, TAKEOFF or flight permission.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT safety officer/commander/MERIVUS administrator account.")
        : action == QStringLiteral("task.observation.close.approve") ? QStringLiteral(
        "Approve task.observation.close ONLY: end this local monitor/release its local reservation. Aircraft MAY STILL FLY. NO STOP/LAND/DISARM; NOT Completed.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT approver account.")
        : action == QStringLiteral("mission.start.ground.reassess.approve") ? QStringLiteral(
        "NEW ground-reassessment CONDITIONAL approval; old authority NOT restored. NO ARM/mode/start or upload lease. READ deadline NOT renewed.\n%1\nSHA256:%2\nApplicant SID:%3\nDIFFERENT approver account.")
        : action == QStringLiteral("mission.start") ? QStringLiteral(
        "Approve independently reviewed mission.start intent. Later controlled startup MAY ARM/TAKE OFF. This call sends NO flight command; NOT upload authorization/lease.\n%1\nSHA-256: %2\nApplicant SID: %3\nUse a DIFFERENT approval account.") : QStringLiteral(
        "Approve mission.upload; NO ARM/start. Lease max4min, capped by existing operator login.\n%1\nBinding SHA-256: %2\nApplicant SID: %3\nUse DIFFERENT safety officer/commander/MERIVUS administrator.");
    const NativeIdentity native = promptIdentity(prompt
        .arg(operationSummary, bindingHash, applicantIdentity.value(QStringLiteral("actorId")).toString()));
    if (!self) return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("session-destroyed")}};
    _promptActive = false;
    QString error = native.error;
    if (!approverOutboundAllowed(this, action) || !identitySnapshotCurrent(applicantIdentity))
        error = QStringLiteral("applicant-source-changed");
    if (!self) return denied(QStringLiteral("session-destroyed"));
    if (!approverOutboundAllowed(this, action)) error = QStringLiteral("applicant-source-changed");
    if (_clock.elapsed() >= expires) error = QStringLiteral("approver-authentication-expired");
    else if (error.isEmpty() && native.actorId == applicantIdentity.value(QStringLiteral("actorId")).toString())
        error = QStringLiteral("independent-approver-required");
    else if (error.isEmpty() && (!approverRole(native.role) || currentRole(native.actorId) != native.role))
        error = QStringLiteral("independent-approver-role-unavailable");
    if (!self) return denied(QStringLiteral("session-destroyed"));
    if (!approverOutboundAllowed(this, action) || !identitySnapshotCurrent(applicantIdentity))
        error = QStringLiteral("applicant-source-changed");
    if (!self) return denied(QStringLiteral("session-destroyed"));
    if (!approverOutboundAllowed(this, action)) error = QStringLiteral("applicant-source-changed");
    if (_clock.elapsed() >= expires) error = QStringLiteral("approver-authentication-expired");
    if (!error.isEmpty()) {
        _lastError = error;
        const QVariantMap failure = denied(error);
        emit changed();
        return failure;
    }
    _approvedApplicantIdentity = applicantIdentity;
    _approverReceipt = {{QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("sessionId"), uuid()},
        {QStringLiteral("sessionEpoch"), _sessionEpoch}, {QStringLiteral("sessionRevision"), QString::number(_sessionRevision)},
        {QStringLiteral("actorId"), native.actorId}, {QStringLiteral("displayName"), native.displayName},
        {QStringLiteral("role"), native.role}, {QStringLiteral("authenticationSource"), AuthenticationSource},
        {QStringLiteral("authenticated"), true}, {QStringLiteral("issuedAtMonotonicMs"), double(issued)},
        {QStringLiteral("expiresAtMonotonicMs"), double(expires)}, {QStringLiteral("action"), action},
        {QStringLiteral("bindingHash"), bindingHash},
        {QStringLiteral("applicantSessionId"), applicantIdentity.value(QStringLiteral("sessionId"))},
        {QStringLiteral("applicantSessionRevision"), applicantIdentity.value(QStringLiteral("sessionRevision"))},
        {QStringLiteral("applicantActorId"), applicantIdentity.value(QStringLiteral("actorId"))}};
    _lastError.clear();
    const QVariantMap receipt = _approverReceipt;
    emit changed();
    if (!self || !approverOutboundAllowed(this, action) || !approverSnapshotCurrent(receipt)
        || !self || !approverOutboundAllowed(this, action))
        return {{QStringLiteral("ok"), false}, {QStringLiteral("reason"), QStringLiteral("approver-source-changed")}};
    return {{QStringLiteral("ok"), true}, {QStringLiteral("identityReceipt"), receipt},
        {QStringLiteral("flightCommandReleased"), false}};
#endif
}

bool FleetOperatorSession::approverSnapshotCurrent(const QVariantMap& receipt) const
{
    const QVariantMap frozen = receipt;
    const QString action = frozen.value(QStringLiteral("action")).toString();
    QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    if (!ownerThread() || _promptActive || frozen.size() != 16 || !approverOutboundAllowed(this, action)
        || !identitySnapshotCurrent(_approvedApplicantIdentity)) return false;
    if (!self || !approverOutboundAllowed(this, action)) return false;
    QVariantMap identity = frozen;
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("bindingHash"),
        QStringLiteral("applicantSessionId"), QStringLiteral("applicantSessionRevision"), QStringLiteral("applicantActorId")}) {
        if (frozen.value(key).type() != QVariant::String) return false;
        identity.remove(key);
    }
    if (!identityShape(identity) || canonicalReceiptNumbers(frozen) != _approverReceipt) return false;
    const QString actor = frozen.value(QStringLiteral("actorId")).toString();
    const QString role = frozen.value(QStringLiteral("role")).toString();
    const QString nativeRole = currentRole(actor);
    if (!self || !approverOutboundAllowed(this, action) || !workstationCurrent() || !self
        || !approverOutboundAllowed(this, action)) return false;
    return self && approverOutboundAllowed(this, action)
        && canonicalReceiptNumbers(frozen) == _approverReceipt && approverRole(role) && nativeRole == role
        && actor != _approvedApplicantIdentity.value(QStringLiteral("actorId")).toString()
        && frozen.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && frozen.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && approvalAction(frozen.value(QStringLiteral("action")).toString())
        && _clock.elapsed() < frozen.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && frozen.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
            - frozen.value(QStringLiteral("issuedAtMonotonicMs")).toDouble() <= ApproverLifetimeMs;
}

bool FleetOperatorSession::consumeObservationCloseAuthentication(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, FleetMissionObservationClose* actualOwner, const QString& requestedBindingHash)
{
    // Own all borrowed values before a WTS/SAM query can dispatch callbacks
    // that destroy or replace the caller. No close-purpose lease is retained.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QString bindingHash = requestedBindingHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetMissionObservationClose> owner(actualOwner);
    if (!ownerThread() || _promptActive || !owner || owner->thread() != thread()
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")).type() != QVariant::String
        || approver.value(QStringLiteral("action")).toString() != QStringLiteral("task.observation.close.approve")
        || approver.value(QStringLiteral("bindingHash")).type() != QVariant::String
        || approver.value(QStringLiteral("bindingHash")).toString() != bindingHash) return false;
    const QString epoch = _sessionEpoch;
    const quint64 revision = _sessionRevision;
    const QVariantMap actualPrimary = _identityReceipt, actualApprover = _approverReceipt;
    const QVariantMap actualApplicant = _approvedApplicantIdentity;
    const auto unchanged = [&]() {
        return self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
            && _sessionEpoch == epoch && _sessionRevision == revision
            && _identityReceipt == actualPrimary && _approverReceipt == actualApprover
            && _approvedApplicantIdentity == actualApplicant;
    };
    const auto bindingCurrent = [&]() {
        if (!unchanged()) return false;
        // This exact friend callback is a passive frozen-context comparison:
        // no OS validation, public notifications, or recursive authentication.
        const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
        return matches && unchanged();
    };
    if (!bindingCurrent() || !identitySnapshotCurrent(primary) || !bindingCurrent()
        || !approverSnapshotCurrent(approver) || !bindingCurrent()
        || !identitySnapshotCurrent(primary) || !bindingCurrent()) return false;
    // Snapshot-current gates establish both real SIDs/roles, their original
    // login/prompt deadlines and the current active/unlocked workstation.
    // Check time again after those queries; none of their cost renews a TTL.
    if (_clock.elapsed() >= primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        || _clock.elapsed() >= approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) return false;
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    // Burn first, then finish the passive owner fence. Even a failed final
    // fence cannot restore this identity or create a replayable grant.
    if (!self || !owner || !ownerThread() || _promptActive || owner->thread() != thread()
        || _sessionEpoch != epoch || _sessionRevision != revision || _identityReceipt != actualPrimary
        || !_approverReceipt.isEmpty() || !_approvedApplicantIdentity.isEmpty()) return false;
    const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
    // No synchronous changed, new proof/lease, or flight-command operation.
    return matches && self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
        && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
        && _approverReceipt.isEmpty() && _approvedApplicantIdentity.isEmpty()
        && _clock.elapsed() < primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && _clock.elapsed() < approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::consumeOccupancyRecoveryAuthentication(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, FleetOccupancyRecoveryReview* actualOwner, const QString& requestedBindingHash)
{
    // Borrowed values may belong to a caller destroyed by WTS/SAM/modal
    // callbacks. Own them first; only the actual native review may consume.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QString bindingHash = requestedBindingHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetOccupancyRecoveryReview> owner(actualOwner);
    if (!ownerThread() || _promptActive || !owner || owner->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")).type() != QVariant::String
        || approver.value(QStringLiteral("action")).toString() != QStringLiteral("fleet.occupancy.recover.approve")
        || approver.value(QStringLiteral("bindingHash")).type() != QVariant::String
        || approver.value(QStringLiteral("bindingHash")).toString() != bindingHash) return false;
    const QString epoch = _sessionEpoch;
    const quint64 revision = _sessionRevision;
    const QVariantMap actualPrimary = _identityReceipt, actualApprover = _approverReceipt;
    const QVariantMap actualApplicant = _approvedApplicantIdentity;
    const auto unchanged = [&]() {
        return self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
            && _sessionEpoch == epoch && _sessionRevision == revision
            && _identityReceipt == actualPrimary && _approverReceipt == actualApprover
            && _approvedApplicantIdentity == actualApplicant;
    };
    const auto bindingCurrent = [&]() {
        if (!unchanged()) return false;
        // A passive actual-owner memory fence only: no OS query, notify,
        // QML callback, public current() or recursive identity check.
        const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
        return matches && unchanged();
    };
    if (!bindingCurrent() || !identitySnapshotCurrent(primary) || !bindingCurrent()
        || !approverSnapshotCurrent(approver) || !bindingCurrent()
        || !identitySnapshotCurrent(primary) || !bindingCurrent()) return false;
    if (_clock.elapsed() >= primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        || _clock.elapsed() >= approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) return false;
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    // Failed post-consumption source checks never resurrect this approval.
    // No persistent proof, upload/start authority, permission or lease exists.
    if (!self || !owner || !ownerThread() || _promptActive || owner->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _sessionEpoch != epoch || _sessionRevision != revision || _identityReceipt != actualPrimary
        || !_approverReceipt.isEmpty() || !_approvedApplicantIdentity.isEmpty()) return false;
    const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
    return matches && self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
        && _approverReceipt.isEmpty() && _approvedApplicantIdentity.isEmpty()
        && _clock.elapsed() < primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && _clock.elapsed() < approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::occupancyRecoveryConsumedIdentityCurrent(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover) const
{
    // Called only by the actual friend Review after its native one-use consume.
    // No identityCurrent/workstation/SAM/source getter, callback or signal: the
    // remaining original TTL must be checked after later native source reads.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    if (!ownerThread() || _promptActive || _revisionExhausted || !identityShape(primary)
        || canonicalReceiptNumbers(primary) != _identityReceipt
        || primary.value(QStringLiteral("sessionEpoch")).toString() != _sessionEpoch
        || primary.value(QStringLiteral("sessionRevision")).toString() != QString::number(_sessionRevision)
        || !_approverReceipt.isEmpty() || !_approvedApplicantIdentity.isEmpty() || approver.size() != 16) return false;
    QVariantMap approverIdentity = approver;
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("bindingHash"),
        QStringLiteral("applicantSessionId"), QStringLiteral("applicantSessionRevision"), QStringLiteral("applicantActorId")}) {
        if (approver.value(key).type() != QVariant::String || approver.value(key).isNull()) return false;
        approverIdentity.remove(key);
    }
    if (!identityShape(approverIdentity)
        || approver.value(QStringLiteral("action")).toString() != QStringLiteral("fleet.occupancy.recover.approve")
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(approver.value(QStringLiteral("bindingHash")).toString()).hasMatch()
        || approver.value(QStringLiteral("sessionEpoch")).toString() != _sessionEpoch
        || approver.value(QStringLiteral("sessionRevision")).toString() != QString::number(_sessionRevision)
        || approver.value(QStringLiteral("applicantSessionId")) != primary.value(QStringLiteral("sessionId"))
        || approver.value(QStringLiteral("applicantSessionRevision")) != primary.value(QStringLiteral("sessionRevision"))
        || approver.value(QStringLiteral("applicantActorId")) != primary.value(QStringLiteral("actorId"))
        || approver.value(QStringLiteral("actorId")) == primary.value(QStringLiteral("actorId"))
        || !approverRole(approver.value(QStringLiteral("role")).toString())) return false;
    const QString approverSessionId = approver.value(QStringLiteral("sessionId")).toString();
    if (QUuid(approverSessionId).toString(QUuid::WithoutBraces) != approverSessionId) return false;
    const double primaryIssued = primary.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double primaryExpires = primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const double approverIssued = approver.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double approverExpires = approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const qint64 now = _clock.elapsed();
    return approverIssued >= primaryIssued && approverExpires > approverIssued
        && approverExpires - approverIssued <= ApproverLifetimeMs && approverExpires <= primaryExpires
        && now >= primaryIssued && now >= approverIssued && now < primaryExpires && now < approverExpires;
}

bool FleetOperatorSession::consumeFirmwareCatalogReloadAuthentication(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, FleetFirmwareCatalogReload* actualOwner, const QString& requestedBindingHash)
{
    // Own caller values before account/workstation queries can dispatch
    // callbacks. Only the actual native Reload may consume this purpose.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QString bindingHash = requestedBindingHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetFirmwareCatalogReload> owner(actualOwner);
    if (!ownerThread() || _promptActive || !owner || owner->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")).type() != QVariant::String
        || approver.value(QStringLiteral("action")).toString() != QStringLiteral("firmware.catalog.reload.approve")
        || approver.value(QStringLiteral("bindingHash")).type() != QVariant::String
        || approver.value(QStringLiteral("bindingHash")).toString() != bindingHash) return false;
    const QString epoch = _sessionEpoch;
    const quint64 revision = _sessionRevision;
    const QVariantMap actualPrimary = _identityReceipt, actualApprover = _approverReceipt;
    const QVariantMap actualApplicant = _approvedApplicantIdentity;
    const auto unchanged = [&]() {
        return self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
            && _sessionEpoch == epoch && _sessionRevision == revision
            && _identityReceipt == actualPrimary && _approverReceipt == actualApprover
            && _approvedApplicantIdentity == actualApplicant;
    };
    const auto bindingCurrent = [&]() {
        if (!unchanged()) return false;
        // Exact friend-owned frozen-context comparison: no QML, OS query,
        // signal, public current() or recursive authentication in this tail.
        const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
        return matches && unchanged();
    };
    if (!bindingCurrent() || !identitySnapshotCurrent(primary) || !bindingCurrent()
        || !approverSnapshotCurrent(approver) || !bindingCurrent()
        || !identitySnapshotCurrent(primary) || !bindingCurrent()) return false;
    if (_clock.elapsed() >= primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        || _clock.elapsed() >= approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) return false;
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    // Burn once before the final passive owner check. A later failure does
    // not restore an approval or manufacture any lease/flight permission.
    if (!self || !owner || !ownerThread() || _promptActive || owner->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _sessionEpoch != epoch || _sessionRevision != revision || _identityReceipt != actualPrimary
        || !_approverReceipt.isEmpty() || !_approvedApplicantIdentity.isEmpty()) return false;
    const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
    return matches && self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
        && _approverReceipt.isEmpty() && _approvedApplicantIdentity.isEmpty()
        && _clock.elapsed() < primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && _clock.elapsed() < approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::firmwareCatalogReloadConsumedIdentityCurrent(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover) const
{
    // Only the actual friend Reload holds the successful consumed fact and
    // frozen binding. No SAM/WTS/identityCurrent query or signal occurs here.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    if (!ownerThread() || _promptActive || _revisionExhausted
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !identityShape(primary)
        || canonicalReceiptNumbers(primary) != _identityReceipt
        || primary.value(QStringLiteral("sessionEpoch")).toString() != _sessionEpoch
        || primary.value(QStringLiteral("sessionRevision")).toString() != QString::number(_sessionRevision)
        || !_approverReceipt.isEmpty() || !_approvedApplicantIdentity.isEmpty() || approver.size() != 16) return false;
    QVariantMap approverIdentity = approver;
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("bindingHash"),
        QStringLiteral("applicantSessionId"), QStringLiteral("applicantSessionRevision"), QStringLiteral("applicantActorId")}) {
        if (approver.value(key).type() != QVariant::String || approver.value(key).isNull()) return false;
        approverIdentity.remove(key);
    }
    if (!identityShape(approverIdentity)
        || approver.value(QStringLiteral("action")).toString() != QStringLiteral("firmware.catalog.reload.approve")
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(approver.value(QStringLiteral("bindingHash")).toString()).hasMatch()
        || approver.value(QStringLiteral("sessionEpoch")).toString() != _sessionEpoch
        || approver.value(QStringLiteral("sessionRevision")).toString() != QString::number(_sessionRevision)
        || approver.value(QStringLiteral("applicantSessionId")) != primary.value(QStringLiteral("sessionId"))
        || approver.value(QStringLiteral("applicantSessionRevision")) != primary.value(QStringLiteral("sessionRevision"))
        || approver.value(QStringLiteral("applicantActorId")) != primary.value(QStringLiteral("actorId"))
        || approver.value(QStringLiteral("actorId")) == primary.value(QStringLiteral("actorId"))
        || !approverRole(approver.value(QStringLiteral("role")).toString())) return false;
    const QString approverSessionId = approver.value(QStringLiteral("sessionId")).toString();
    if (QUuid(approverSessionId).toString(QUuid::WithoutBraces) != approverSessionId) return false;
    const double primaryIssued = primary.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double primaryExpires = primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const double approverIssued = approver.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double approverExpires = approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const qint64 now = _clock.elapsed();
    return FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        && approverIssued >= primaryIssued && approverExpires > approverIssued
        && approverExpires - approverIssued <= ApproverLifetimeMs && approverExpires <= primaryExpires
        && now >= primaryIssued && now >= approverIssued && now < primaryExpires && now < approverExpires;
}

bool FleetOperatorSession::consumeCapabilityReviewAuthentication(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, FleetCapabilityReview* actualOwner, const QString& requestedBindingHash)
{
    // Own caller values before account/workstation queries can dispatch
    // callbacks. This purpose belongs only to the actual native Review.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QString bindingHash = requestedBindingHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetCapabilityReview> owner(actualOwner);
    if (!ownerThread() || _promptActive || !owner || owner->thread() != thread()
        || owner->_session.data() != this || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return false;
    const QString purpose = owner->authenticationPurpose();
    if ((purpose != QStringLiteral("fleet.capability.review.approve") && purpose != QStringLiteral("fleet.configuration.review.approve")
            && purpose != QStringLiteral("fleet.performance.declaration.approve"))
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")).type() != QVariant::String
        || approver.value(QStringLiteral("action")).toString() != purpose
        || approver.value(QStringLiteral("bindingHash")).type() != QVariant::String
        || approver.value(QStringLiteral("bindingHash")).toString() != bindingHash) return false;
    const QString epoch = _sessionEpoch;
    const quint64 revision = _sessionRevision;
    const QVariantMap actualPrimary = _identityReceipt, actualApprover = _approverReceipt;
    const QVariantMap actualApplicant = _approvedApplicantIdentity;
    const auto unchanged = [&]() {
        return self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
            && owner->_session.data() == this && owner->authenticationPurpose() == purpose
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
            && _sessionEpoch == epoch && _sessionRevision == revision
            && _identityReceipt == actualPrimary && _approverReceipt == actualApprover
            && _approvedApplicantIdentity == actualApplicant;
    };
    const auto bindingCurrent = [&]() {
        if (!unchanged()) return false;
        // Pure friend-owned binding: no QML, account lookup or signal here.
        const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
        return matches && unchanged();
    };
    if (!bindingCurrent() || !identitySnapshotCurrent(primary) || !bindingCurrent()
        || !approverSnapshotCurrent(approver) || !bindingCurrent()
        || !identitySnapshotCurrent(primary) || !bindingCurrent()) return false;
    if (_clock.elapsed() >= primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        || _clock.elapsed() >= approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) return false;
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    // One use, even if a later source check fails; never restore approval.
    if (!self || !owner || !ownerThread() || _promptActive || owner->thread() != thread()
        || owner->_session.data() != this || owner->authenticationPurpose() != purpose
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || _sessionEpoch != epoch || _sessionRevision != revision || _identityReceipt != actualPrimary
        || !_approverReceipt.isEmpty() || !_approvedApplicantIdentity.isEmpty()) return false;
    const bool matches = owner->authenticationBindingCurrent(this, primary, approver, bindingHash);
    return matches && self && owner && ownerThread() && !_promptActive && owner->thread() == thread()
        && owner->_session.data() == this && owner->authenticationPurpose() == purpose
        && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
        && _approverReceipt.isEmpty() && _approvedApplicantIdentity.isEmpty()
        && _clock.elapsed() < primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && _clock.elapsed() < approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::capabilityReviewConsumedIdentityCurrent(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, const FleetCapabilityReview* actualOwner) const
{
    // Only the friend Review proves its actual one-use consumption and binding.
    // This final fence deliberately performs no account/source callbacks.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QPointer<FleetCapabilityReview> owner(const_cast<FleetCapabilityReview*>(actualOwner));
    if (!ownerThread() || _promptActive || _revisionExhausted
        || !owner || owner->thread() != thread() || owner->_session.data() != this
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !identityShape(primary)
        || canonicalReceiptNumbers(primary) != _identityReceipt
        || primary.value(QStringLiteral("sessionEpoch")).toString() != _sessionEpoch
        || primary.value(QStringLiteral("sessionRevision")).toString() != QString::number(_sessionRevision)
        || !_approverReceipt.isEmpty() || !_approvedApplicantIdentity.isEmpty() || approver.size() != 16) return false;
    const QString purpose = owner->authenticationPurpose();
    if (purpose != QStringLiteral("fleet.capability.review.approve") && purpose != QStringLiteral("fleet.configuration.review.approve")
        && purpose != QStringLiteral("fleet.performance.declaration.approve")) return false;
    QVariantMap approverIdentity = approver;
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("bindingHash"),
        QStringLiteral("applicantSessionId"), QStringLiteral("applicantSessionRevision"), QStringLiteral("applicantActorId")}) {
        if (approver.value(key).type() != QVariant::String || approver.value(key).isNull()) return false;
        approverIdentity.remove(key);
    }
    if (!identityShape(approverIdentity)
        || approver.value(QStringLiteral("action")).toString() != purpose
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(approver.value(QStringLiteral("bindingHash")).toString()).hasMatch()
        || approver.value(QStringLiteral("sessionEpoch")).toString() != _sessionEpoch
        || approver.value(QStringLiteral("sessionRevision")).toString() != QString::number(_sessionRevision)
        || approver.value(QStringLiteral("applicantSessionId")) != primary.value(QStringLiteral("sessionId"))
        || approver.value(QStringLiteral("applicantSessionRevision")) != primary.value(QStringLiteral("sessionRevision"))
        || approver.value(QStringLiteral("applicantActorId")) != primary.value(QStringLiteral("actorId"))
        || approver.value(QStringLiteral("actorId")) == primary.value(QStringLiteral("actorId"))
        || !approverRole(approver.value(QStringLiteral("role")).toString())) return false;
    const QString approverSessionId = approver.value(QStringLiteral("sessionId")).toString();
    if (QUuid(approverSessionId).toString(QUuid::WithoutBraces) != approverSessionId) return false;
    const double primaryIssued = primary.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double primaryExpires = primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const double approverIssued = approver.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double approverExpires = approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    const qint64 now = _clock.elapsed();
    return FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        && owner && owner->thread() == thread() && owner->_session.data() == this && owner->authenticationPurpose() == purpose
        && approverIssued >= primaryIssued && approverExpires > approverIssued
        && approverExpires - approverIssued <= ApproverLifetimeMs && approverExpires <= primaryExpires
        && now >= primaryIssued && now >= approverIssued && now < primaryExpires && now < approverExpires;
}

bool FleetOperatorSession::consumeTelemetryConfigurationAuthentication(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, FleetOptionalTelemetryConfiguration* actualOwner,
    const QString& requestedHash, int maximumRemainingMs)
{
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QString bindingHash = requestedHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetOptionalTelemetryConfiguration> owner(actualOwner);
    if (!ownerThread() || _promptActive || _revisionExhausted || !owner || owner->thread() != thread()
        || owner->_session.data() != this || !_telemetryConfigurationBindingHash.isEmpty()
        || maximumRemainingMs <= 0 || maximumRemainingMs > 30000
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")) != QVariant(QStringLiteral("fleet.telemetry.interval.configure.approve"))
        || approver.value(QStringLiteral("bindingHash")) != QVariant(bindingHash)) return false;
    const qint64 enteredAt = _clock.elapsed();
    if (enteredAt < 0 || enteredAt > std::numeric_limits<qint64>::max() - maximumRemainingMs) return false;
    const qint64 localDeadline = enteredAt + maximumRemainingMs;
    const QString epoch = _sessionEpoch;
    const quint64 revision = _sessionRevision;
    const QVariantMap actualPrimary = _identityReceipt, actualApprover = _approverReceipt;
    const QVariantMap actualApplicant = _approvedApplicantIdentity;
    const auto unchanged = [&]() {
        return self && owner && ownerThread() && !_promptActive && !_revisionExhausted
            && owner->thread() == thread() && owner->_session.data() == this
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
            && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
            && _approverReceipt == actualApprover && _approvedApplicantIdentity == actualApplicant
            && _telemetryConfigurationBindingHash.isEmpty() && _clock.elapsed() < localDeadline;
    };
    const auto bound = [&]() {
        return unchanged() && owner->authenticationBindingCurrent(this, primary, approver, bindingHash) && unchanged();
    };
    if (!bound() || !identitySnapshotCurrent(primary) || !bound()
        || !approverSnapshotCurrent(approver) || !bound() || !identitySnapshotCurrent(primary) || !bound()) return false;
    const qint64 deadline = qMin(localDeadline, qMin(qint64(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
        qint64(approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())));
    if (_clock.elapsed() >= deadline) return false;
    _approverReceipt.clear(); _approvedApplicantIdentity.clear(); // Burn once, even if the passive tail rejects.
    const auto consumed = [&]() {
        return self && owner && ownerThread() && !_promptActive && !_revisionExhausted
            && owner->thread() == thread() && owner->_session.data() == this
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
            && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
            && _approverReceipt.isEmpty() && _approvedApplicantIdentity.isEmpty()
            && _telemetryConfigurationBindingHash.isEmpty() && _clock.elapsed() < deadline;
    };
    if (!consumed() || !owner->authenticationBindingCurrent(this, primary, approver, bindingHash) || !consumed()) return false;
    // Private typed-owner scope, never an exported/restorable lease. The
    // original account and capture deadlines remain independently enforced.
    _telemetryConfigurationOwner = owner;
    _telemetryConfigurationPrimary = primary;
    _telemetryConfigurationApprover = approver;
    _telemetryConfigurationBindingHash = bindingHash;
    _telemetryConfigurationDeadlineMs = deadline;
    return true;
}

bool FleetOperatorSession::telemetryConfigurationConsumedIdentityCurrent(
    const FleetOptionalTelemetryConfiguration* actualOwner) const
{
    const QPointer<FleetOptionalTelemetryConfiguration> owner(const_cast<FleetOptionalTelemetryConfiguration*>(actualOwner));
    if (!ownerThread() || _promptActive || _revisionExhausted || !owner || owner != _telemetryConfigurationOwner
        || owner->thread() != thread() || owner->_session.data() != this || !owner->_authenticationConsumed
        || !owner->memoryCurrent(owner->_generation) || !owner->_operationConfirmed
        || owner->_bindingHash != _telemetryConfigurationBindingHash || owner->_identity != _telemetryConfigurationPrimary
        || owner->_approver != _telemetryConfigurationApprover || _telemetryConfigurationBindingHash.isEmpty()
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !identityShape(_telemetryConfigurationPrimary)
        || canonicalReceiptNumbers(_telemetryConfigurationPrimary) != _identityReceipt
        || _clock.elapsed() < 0 || _clock.elapsed() >= _telemetryConfigurationDeadlineMs) return false;
    const QVariantMap primary = _telemetryConfigurationPrimary, approver = _telemetryConfigurationApprover;
    if (approver.size() != 16 || approver.value(QStringLiteral("action"))
            != QVariant(QStringLiteral("fleet.telemetry.interval.configure.approve"))
        || approver.value(QStringLiteral("bindingHash")) != QVariant(_telemetryConfigurationBindingHash)
        || approver.value(QStringLiteral("sessionEpoch")) != QVariant(_sessionEpoch)
        || approver.value(QStringLiteral("sessionRevision")) != QVariant(QString::number(_sessionRevision))
        || approver.value(QStringLiteral("applicantSessionId")) != primary.value(QStringLiteral("sessionId"))
        || approver.value(QStringLiteral("applicantSessionRevision")) != primary.value(QStringLiteral("sessionRevision"))
        || approver.value(QStringLiteral("applicantActorId")) != primary.value(QStringLiteral("actorId"))
        || approver.value(QStringLiteral("actorId")) == primary.value(QStringLiteral("actorId"))
        || !approverRole(approver.value(QStringLiteral("role")).toString())) return false;
    QVariantMap identity = approver;
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("bindingHash"), QStringLiteral("applicantSessionId"),
        QStringLiteral("applicantSessionRevision"), QStringLiteral("applicantActorId")}) identity.remove(key);
    const double issued = approver.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double expires = approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    return identityShape(identity) && issued >= primary.value(QStringLiteral("issuedAtMonotonicMs")).toDouble()
        && expires > issued && expires - issued <= ApproverLifetimeMs
        && expires <= primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && _clock.elapsed() >= issued && _clock.elapsed() < expires;
}

bool FleetOperatorSession::telemetryConfigurationAuthenticationCurrent(
    const FleetOptionalTelemetryConfiguration* actualOwner) const
{
    const QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QPointer<FleetOptionalTelemetryConfiguration> owner(const_cast<FleetOptionalTelemetryConfiguration*>(actualOwner));
    if (!telemetryConfigurationConsumedIdentityCurrent(owner.data())) return false;
    const QVariantMap primary = _telemetryConfigurationPrimary, approver = _telemetryConfigurationApprover;
    const QString hash = _telemetryConfigurationBindingHash;
    const auto same = [&]() {
        return self && owner && _telemetryConfigurationPrimary == primary && _telemetryConfigurationApprover == approver
            && _telemetryConfigurationBindingHash == hash && telemetryConfigurationConsumedIdentityCurrent(owner.data());
    };
    if (!identitySnapshotCurrent(primary) || !same()) return false;
    const QString role = currentRole(approver.value(QStringLiteral("actorId")).toString());
    if (!same() || role != approver.value(QStringLiteral("role")).toString() || !workstationCurrent() || !same()) return false;
    return same();
}

bool FleetOperatorSession::revokeTelemetryConfigurationAuthentication(FleetOptionalTelemetryConfiguration* actualOwner,
    const QString& bindingHash)
{
    if (!ownerThread() || !actualOwner || _telemetryConfigurationOwner.data() != actualOwner
        || _telemetryConfigurationBindingHash != bindingHash || bindingHash.isEmpty()) return false;
    clearTelemetryConfigurationAuthentication();
    return true;
}

void FleetOperatorSession::clearTelemetryConfigurationAuthentication()
{
    _telemetryConfigurationOwner.clear(); _telemetryConfigurationPrimary.clear(); _telemetryConfigurationApprover.clear();
    _telemetryConfigurationBindingHash.clear(); _telemetryConfigurationDeadlineMs = -1;
}

bool FleetOperatorSession::consumeCameraStreamControlAuthentication(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, FleetCameraStreamControl* actualOwner,
    const QString& requestedHash, int maximumRemainingMs)
{
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QString bindingHash = requestedHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetCameraStreamControl> owner(actualOwner);
    if (!ownerThread() || _promptActive || _revisionExhausted || !owner || owner->thread() != thread()
        || owner->_session.data() != this || !_cameraStreamControlBindingHash.isEmpty()
        || maximumRemainingMs <= 0 || maximumRemainingMs > 30000
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")) != QVariant(QStringLiteral("fleet.video.stream.control.approve"))
        || approver.value(QStringLiteral("bindingHash")) != QVariant(bindingHash)) return false;
    const qint64 enteredAt = _clock.elapsed();
    if (enteredAt < 0 || enteredAt > std::numeric_limits<qint64>::max() - maximumRemainingMs) return false;
    const qint64 localDeadline = enteredAt + maximumRemainingMs;
    const QString epoch = _sessionEpoch;
    const quint64 revision = _sessionRevision;
    const QVariantMap actualPrimary = _identityReceipt, actualApprover = _approverReceipt;
    const QVariantMap actualApplicant = _approvedApplicantIdentity;
    const auto unchanged = [&]() {
        return self && owner && ownerThread() && !_promptActive && !_revisionExhausted
            && owner->thread() == thread() && owner->_session.data() == this
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
            && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
            && _approverReceipt == actualApprover && _approvedApplicantIdentity == actualApplicant
            && _cameraStreamControlBindingHash.isEmpty() && _clock.elapsed() < localDeadline;
    };
    const auto bound = [&]() {
        return unchanged() && owner->authenticationBindingCurrent(this, primary, approver, bindingHash) && unchanged();
    };
    if (!bound() || !identitySnapshotCurrent(primary) || !bound()
        || !approverSnapshotCurrent(approver) || !bound() || !identitySnapshotCurrent(primary) || !bound()) return false;
    const qint64 deadline = qMin(localDeadline, qMin(qint64(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
        qint64(approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())));
    if (_clock.elapsed() >= deadline) return false;
    _approverReceipt.clear(); _approvedApplicantIdentity.clear(); // Burn once, even if the passive tail rejects.
    const auto consumed = [&]() {
        return self && owner && ownerThread() && !_promptActive && !_revisionExhausted
            && owner->thread() == thread() && owner->_session.data() == this
            && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
            && _sessionEpoch == epoch && _sessionRevision == revision && _identityReceipt == actualPrimary
            && _approverReceipt.isEmpty() && _approvedApplicantIdentity.isEmpty()
            && _cameraStreamControlBindingHash.isEmpty() && _clock.elapsed() < deadline;
    };
    if (!consumed() || !owner->authenticationBindingCurrent(this, primary, approver, bindingHash) || !consumed()) return false;
    // Private typed-owner scope, never an exported/restorable lease. The
    // original account and control-review deadlines remain independently enforced.
    _cameraStreamControlOwner = owner;
    _cameraStreamControlPrimary = primary;
    _cameraStreamControlApprover = approver;
    _cameraStreamControlBindingHash = bindingHash;
    _cameraStreamControlDeadlineMs = deadline;
    return true;
}

bool FleetOperatorSession::cameraStreamControlConsumedIdentityCurrent(
    const FleetCameraStreamControl* actualOwner) const
{
    const QPointer<FleetCameraStreamControl> owner(const_cast<FleetCameraStreamControl*>(actualOwner));
    if (!ownerThread() || _promptActive || _revisionExhausted || !owner || owner != _cameraStreamControlOwner
        || owner->thread() != thread() || owner->_session.data() != this || !owner->_authenticationConsumed
        || !owner->memoryCurrent(owner->_generation) || !owner->_operationConfirmed
        || owner->_bindingHash != _cameraStreamControlBindingHash || owner->_identity != _cameraStreamControlPrimary
        || owner->_approver != _cameraStreamControlApprover || _cameraStreamControlBindingHash.isEmpty()
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !identityShape(_cameraStreamControlPrimary)
        || canonicalReceiptNumbers(_cameraStreamControlPrimary) != _identityReceipt
        || _clock.elapsed() < 0 || _clock.elapsed() >= _cameraStreamControlDeadlineMs) return false;
    const QVariantMap primary = _cameraStreamControlPrimary, approver = _cameraStreamControlApprover;
    if (approver.size() != 16 || approver.value(QStringLiteral("action"))
            != QVariant(QStringLiteral("fleet.video.stream.control.approve"))
        || approver.value(QStringLiteral("bindingHash")) != QVariant(_cameraStreamControlBindingHash)
        || approver.value(QStringLiteral("sessionEpoch")) != QVariant(_sessionEpoch)
        || approver.value(QStringLiteral("sessionRevision")) != QVariant(QString::number(_sessionRevision))
        || approver.value(QStringLiteral("applicantSessionId")) != primary.value(QStringLiteral("sessionId"))
        || approver.value(QStringLiteral("applicantSessionRevision")) != primary.value(QStringLiteral("sessionRevision"))
        || approver.value(QStringLiteral("applicantActorId")) != primary.value(QStringLiteral("actorId"))
        || approver.value(QStringLiteral("actorId")) == primary.value(QStringLiteral("actorId"))
        || !approverRole(approver.value(QStringLiteral("role")).toString())) return false;
    QVariantMap identity = approver;
    for (const QString& key : {QStringLiteral("action"), QStringLiteral("bindingHash"), QStringLiteral("applicantSessionId"),
        QStringLiteral("applicantSessionRevision"), QStringLiteral("applicantActorId")}) identity.remove(key);
    const double issued = approver.value(QStringLiteral("issuedAtMonotonicMs")).toDouble();
    const double expires = approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    return identityShape(identity) && issued >= primary.value(QStringLiteral("issuedAtMonotonicMs")).toDouble()
        && expires > issued && expires - issued <= ApproverLifetimeMs
        && expires <= primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && _clock.elapsed() >= issued && _clock.elapsed() < expires;
}

bool FleetOperatorSession::cameraStreamControlAuthenticationCurrent(
    const FleetCameraStreamControl* actualOwner) const
{
    const QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QPointer<FleetCameraStreamControl> owner(const_cast<FleetCameraStreamControl*>(actualOwner));
    if (!cameraStreamControlConsumedIdentityCurrent(owner.data())) return false;
    const QVariantMap primary = _cameraStreamControlPrimary, approver = _cameraStreamControlApprover;
    const QString hash = _cameraStreamControlBindingHash;
    const auto same = [&]() {
        return self && owner && _cameraStreamControlPrimary == primary && _cameraStreamControlApprover == approver
            && _cameraStreamControlBindingHash == hash && cameraStreamControlConsumedIdentityCurrent(owner.data());
    };
    if (!identitySnapshotCurrent(primary) || !same()) return false;
    const QString role = currentRole(approver.value(QStringLiteral("actorId")).toString());
    if (!same() || role != approver.value(QStringLiteral("role")).toString() || !workstationCurrent() || !same()) return false;
    return same();
}

bool FleetOperatorSession::revokeCameraStreamControlAuthentication(FleetCameraStreamControl* actualOwner,
    const QString& bindingHash)
{
    if (!ownerThread() || !actualOwner || _cameraStreamControlOwner.data() != actualOwner
        || _cameraStreamControlBindingHash != bindingHash || bindingHash.isEmpty()) return false;
    clearCameraStreamControlAuthentication();
    return true;
}

void FleetOperatorSession::clearCameraStreamControlAuthentication()
{
    _cameraStreamControlOwner.clear(); _cameraStreamControlPrimary.clear(); _cameraStreamControlApprover.clear();
    _cameraStreamControlBindingHash.clear(); _cameraStreamControlDeadlineMs = -1;
}

void FleetOperatorSession::clearUploadIdentityLease()
{
    _uploadIdentityLease.clear();
    _uploadLeasePrimaryIdentity.clear();
    _uploadLeaseApproverIdentity.clear();
}

QVariantMap FleetOperatorSession::consumeUploadIdentityLease(const QVariantMap& primary, const QVariantMap& approver,
                                                           const QString& requestedLeaseId, int maximumDurationMs)
{
    const QString leaseId = requestedLeaseId;
    QPointer<FleetOperatorSession> self(this);
    const QVariantMap primaryIdentity = primary;
    const QVariantMap approverIdentity = approver;
    if (!ownerThread() || _promptActive || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || maximumDurationMs < 1 || maximumDurationMs > UploadLeaseMaximumMs
        || QUuid(leaseId).isNull() || QUuid(leaseId).toString(QUuid::WithoutBraces) != leaseId
        || approverIdentity.value(QStringLiteral("action")).type() != QVariant::String
        || approverIdentity.value(QStringLiteral("action")).toString() != QStringLiteral("mission.upload")
        || !identitySnapshotCurrent(primaryIdentity) || !self || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !approverSnapshotCurrent(approverIdentity) || !self || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return denied(QStringLiteral("upload-identity-lease-source-invalid"));
    if (!_uploadIdentityLease.isEmpty()) {
        const bool previousCurrent = uploadIdentityLeaseCurrent(_uploadIdentityLease);
        if (!self) return denied(QStringLiteral("session-destroyed"));
        if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return denied(QStringLiteral("upload-identity-lease-source-changed"));
        if (previousCurrent) return denied(QStringLiteral("upload-identity-lease-already-active"));
        clearUploadIdentityLease();
    }
    // Revalidate after an older lease check, then burn the exact approval once.
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !identitySnapshotCurrent(primaryIdentity) || !self
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !approverSnapshotCurrent(approverIdentity) || !self
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return denied(QStringLiteral("upload-identity-lease-source-changed"));
    const qint64 issued = _clock.elapsed();
    const qint64 expires = qMin(issued + maximumDurationMs,
        qint64(primaryIdentity.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()));
    if (expires <= issued || issued >= approverIdentity.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())
        return denied(QStringLiteral("upload-identity-lease-expired"));
    const QVariantMap lease {
        {QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("leaseId"), leaseId},
        {QStringLiteral("sessionEpoch"), _sessionEpoch}, {QStringLiteral("sessionRevision"), QString::number(_sessionRevision)},
        {QStringLiteral("primarySessionId"), primaryIdentity.value(QStringLiteral("sessionId"))},
        {QStringLiteral("primaryActorId"), primaryIdentity.value(QStringLiteral("actorId"))},
        {QStringLiteral("primaryRole"), primaryIdentity.value(QStringLiteral("role"))},
        {QStringLiteral("approverActorId"), approverIdentity.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverRole"), approverIdentity.value(QStringLiteral("role"))},
        {QStringLiteral("approverAuthenticationId"), approverIdentity.value(QStringLiteral("sessionId"))},
        {QStringLiteral("action"), QStringLiteral("mission.upload")},
        {QStringLiteral("bindingHash"), approverIdentity.value(QStringLiteral("bindingHash"))},
        {QStringLiteral("issuedAtMonotonicMs"), double(issued)}, {QStringLiteral("expiresAtMonotonicMs"), double(expires)},
        {QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"), approverIdentity.value(QStringLiteral("issuedAtMonotonicMs")).toDouble()},
        {QStringLiteral("durationMs"), double(expires - issued)}};
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return denied(QStringLiteral("upload-identity-lease-source-changed-before-consumption"));
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    _uploadLeasePrimaryIdentity = canonicalReceiptNumbers(primaryIdentity);
    _uploadLeaseApproverIdentity = canonicalReceiptNumbers(approverIdentity);
    _uploadIdentityLease = lease;
    // No changed signal: acquisition must not synchronously invalidate the
    // approval ledger while it converts this exact receipt to a native grant.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("identityLease"), lease},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::uploadIdentityLeaseCurrent(const QVariantMap& receipt) const
{
    const QVariantMap lease = receipt;
    QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    if (!ownerThread() || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !leaseShape(lease) || canonicalLeaseNumbers(lease) != _uploadIdentityLease
        || !identityCurrent(_uploadLeasePrimaryIdentity, true) || !self
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return false;
    const QString approverRoleNow = currentRole(lease.value(QStringLiteral("approverActorId")).toString());
    if (!self || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !workstationCurrent() || !self
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return false;
    return self && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) && canonicalLeaseNumbers(lease) == _uploadIdentityLease
        && lease.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && lease.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && lease.value(QStringLiteral("primarySessionId")) == _identityReceipt.value(QStringLiteral("sessionId"))
        && lease.value(QStringLiteral("primaryActorId")) == _identityReceipt.value(QStringLiteral("actorId"))
        && lease.value(QStringLiteral("primaryRole")) == _identityReceipt.value(QStringLiteral("role"))
        && lease.value(QStringLiteral("approverActorId")) != lease.value(QStringLiteral("primaryActorId"))
        && approverRole(approverRoleNow) && approverRoleNow == lease.value(QStringLiteral("approverRole")).toString()
        && lease.value(QStringLiteral("approverAuthenticationId")) == _uploadLeaseApproverIdentity.value(QStringLiteral("sessionId"))
        && lease.value(QStringLiteral("bindingHash")) == _uploadLeaseApproverIdentity.value(QStringLiteral("bindingHash"))
        && _uploadLeaseApproverIdentity.value(QStringLiteral("action")).toString() == QStringLiteral("mission.upload")
        && _clock.elapsed() < lease.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && lease.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
            <= _uploadLeasePrimaryIdentity.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::revokeUploadIdentityLease(const QString& leaseId)
{
    if (!ownerThread() || leaseId.isEmpty() || leaseId != _uploadIdentityLease.value(QStringLiteral("leaseId")).toString())
        return false;
    clearUploadIdentityLease();
    return true;
}

void FleetOperatorSession::clearStartApprovalAuthentication()
{
    _startApprovalAuthentication.clear();
    _startApprovalPrimaryIdentity.clear();
    _startApprovalApproverIdentity.clear();
}

QVariantMap FleetOperatorSession::consumeStartApprovalAuthentication(const QVariantMap& requestedPrimary,
                                                                    const QVariantMap& requestedApprover,
                                                                    const QString& requestedBindingHash,
                                                                    const QString& requestedApprovalId)
{
    // Freeze all borrowed caller values before a WTS/native callback can destroy
    // that caller. The returned proof is not a startup permit or an upload lease.
    const QVariantMap primary = requestedPrimary;
    const QVariantMap approver = requestedApprover;
    const QString bindingHash = requestedBindingHash;
    const QString approvalId = requestedApprovalId;
    QPointer<FleetOperatorSession> self(this);
    if (!ownerThread() || _promptActive || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || QUuid(approvalId).isNull() || QUuid(approvalId).toString(QUuid::WithoutBraces) != approvalId
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")).type() != QVariant::String
        || approver.value(QStringLiteral("action")).toString() != QStringLiteral("mission.start")
        || approver.value(QStringLiteral("bindingHash")).type() != QVariant::String
        || approver.value(QStringLiteral("bindingHash")).toString() != bindingHash
        || !identitySnapshotCurrent(primary) || !self || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !approverSnapshotCurrent(approver) || !self || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return startAuthenticationDenied(QStringLiteral("start-approval-authentication-source-invalid"));
    if (!_startApprovalAuthentication.isEmpty()) {
        const bool previousCurrent = startApprovalAuthenticationCurrent(_startApprovalAuthentication);
        if (!self) return startAuthenticationDenied(QStringLiteral("session-destroyed"));
        if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
            return startAuthenticationDenied(QStringLiteral("start-approval-authentication-source-changed"));
        if (previousCurrent)
            return startAuthenticationDenied(QStringLiteral("start-approval-authentication-already-active"));
        clearStartApprovalAuthentication();
    }
    // A previous-proof query can dispatch source changes. Check the exact native
    // pending approval again immediately before its irreversible one-use burn.
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !identitySnapshotCurrent(primary) || !self
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !approverSnapshotCurrent(approver) || !self
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return startAuthenticationDenied(QStringLiteral("start-approval-authentication-source-changed"));
    const qint64 consumed = _clock.elapsed();
    const qint64 expires = qMin(qint64(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
                               qint64(approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()));
    if (expires <= consumed)
        return startAuthenticationDenied(QStringLiteral("start-approval-authentication-expired"));
    const QVariantMap proof {
        {QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("approvalId"), approvalId},
        {QStringLiteral("approverAuthenticationId"), approver.value(QStringLiteral("sessionId"))},
        {QStringLiteral("primarySessionId"), primary.value(QStringLiteral("sessionId"))},
        {QStringLiteral("primaryActorId"), primary.value(QStringLiteral("actorId"))},
        {QStringLiteral("primaryRole"), primary.value(QStringLiteral("role"))},
        {QStringLiteral("approverActorId"), approver.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverRole"), approver.value(QStringLiteral("role"))},
        {QStringLiteral("sessionEpoch"), _sessionEpoch}, {QStringLiteral("sessionRevision"), QString::number(_sessionRevision)},
        {QStringLiteral("action"), QStringLiteral("mission.start")}, {QStringLiteral("bindingHash"), bindingHash},
        {QStringLiteral("consumedAtMonotonicMs"), double(consumed)}, {QStringLiteral("expiresAtMonotonicMs"), double(expires)},
        {QStringLiteral("durationMs"), double(expires - consumed)},
        {QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"), approver.value(QStringLiteral("issuedAtMonotonicMs")).toDouble()}};
    if (!startAuthenticationShape(proof))
        return startAuthenticationDenied(QStringLiteral("start-approval-authentication-shape-invalid"));
    if (!FleetMissionDeploymentEpisode::outboundOwnerAllowed(this))
        return startAuthenticationDenied(QStringLiteral("start-approval-authentication-source-changed-before-consumption"));
    _approverReceipt.clear();
    _approvedApplicantIdentity.clear();
    _startApprovalPrimaryIdentity = canonicalReceiptNumbers(primary);
    _startApprovalApproverIdentity = canonicalReceiptNumbers(approver);
    _startApprovalAuthentication = proof;
    // No synchronous changed: the native start-intent ledger must finish
    // recording this exact authentication basis without callback reentry.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), proof},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::startApprovalAuthenticationCurrent(const QVariantMap& receipt) const
{
    const QVariantMap proof = receipt;
    QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    if (!ownerThread() || _promptActive || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)
        || !startAuthenticationShape(proof)
        || canonicalStartAuthenticationNumbers(proof) != _startApprovalAuthentication) return false;
    const QVariantMap primary = _startApprovalPrimaryIdentity;
    const QVariantMap approver = _startApprovalApproverIdentity;
    if (!identitySnapshotCurrent(primary) || !self || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return false;
    const QString roleNow = currentRole(proof.value(QStringLiteral("approverActorId")).toString());
    if (!self || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) || !workstationCurrent() || !self
        || !FleetMissionDeploymentEpisode::outboundOwnerAllowed(this)) return false;
    return self && FleetMissionDeploymentEpisode::outboundOwnerAllowed(this) && !_promptActive
        && canonicalStartAuthenticationNumbers(proof) == _startApprovalAuthentication
        && primary == _startApprovalPrimaryIdentity && approver == _startApprovalApproverIdentity
        && primary == _identityReceipt
        && proof.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && proof.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && proof.value(QStringLiteral("primarySessionId")) == primary.value(QStringLiteral("sessionId"))
        && proof.value(QStringLiteral("primaryActorId")) == primary.value(QStringLiteral("actorId"))
        && proof.value(QStringLiteral("primaryRole")) == primary.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("approverActorId")) != proof.value(QStringLiteral("primaryActorId"))
        && proof.value(QStringLiteral("approverActorId")) == approver.value(QStringLiteral("actorId"))
        && approverRole(roleNow) && roleNow == proof.value(QStringLiteral("approverRole")).toString()
        && proof.value(QStringLiteral("approverRole")) == approver.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("approverAuthenticationId")) == approver.value(QStringLiteral("sessionId"))
        && approver.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && proof.value(QStringLiteral("bindingHash")) == approver.value(QStringLiteral("bindingHash"))
        && proof.value(QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"))
            == approver.value(QStringLiteral("issuedAtMonotonicMs"))
        && _clock.elapsed() >= proof.value(QStringLiteral("consumedAtMonotonicMs")).toDouble()
        && _clock.elapsed() < proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
            == qMin(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(),
                    approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble());
}

bool FleetOperatorSession::revokeStartApprovalAuthentication(const QString& approvalId)
{
    if (!ownerThread() || approvalId.isEmpty()
        || approvalId != _startApprovalAuthentication.value(QStringLiteral("approvalId")).toString()) return false;
    clearStartApprovalAuthentication();
    return true;
}

void FleetOperatorSession::clearGroundStartApprovalAuthentication()
{
    QObject::disconnect(_groundStartApprovalOwnerDestroyedConnection);
    _groundStartApprovalOwnerDestroyedConnection = {};
    _groundStartApprovalAuthentication.clear();
    _groundStartApprovalPrimaryIdentity.clear();
    _groundStartApprovalApproverIdentity.clear();
    _groundStartApprovalOwner.clear();
}

QVariantMap FleetOperatorSession::consumeGroundStartApprovalAuthentication(const QVariantMap& requestedPrimary,
    const QVariantMap& requestedApprover, QObject* approvalObject, const QString& requestedApprovalId,
    const QString& requestedApprovalEpoch, const QString& requestedBindingHash, const QString& requestedReviewId,
    const QString& requestedReviewEpoch, int maximumRemainingMs)
{
    // All caller members are copied before OS/source callbacks can destroy
    // their owner. The original review budget is fixed BEFORE those callbacks.
    const QVariantMap primary = requestedPrimary, approver = requestedApprover;
    const QString approvalId = requestedApprovalId, approvalEpoch = requestedApprovalEpoch;
    const QString bindingHash = requestedBindingHash, reviewId = requestedReviewId, reviewEpoch = requestedReviewEpoch;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetMissionGroundStartApproval> owner(qobject_cast<FleetMissionGroundStartApproval*>(approvalObject));
    if (!ownerThread() || _promptActive || !owner || owner->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())
        || owner->approvalEpoch() != approvalEpoch || maximumRemainingMs < 1 || maximumRemainingMs > 30000)
        return startAuthenticationDenied(QStringLiteral("ground-conditional-actual-owner-or-budget-required"));
    for (const QString& value : {approvalId, approvalEpoch, reviewId, reviewEpoch})
        if (QUuid(value).isNull() || QUuid(value).toString(QUuid::WithoutBraces) != value)
            return startAuthenticationDenied(QStringLiteral("ground-conditional-exact-scope-required"));
    if (!QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(bindingHash).hasMatch()
        || approver.value(QStringLiteral("action")).type() != QVariant::String
        || approver.value(QStringLiteral("action")).toString() != QStringLiteral("mission.start.ground.reassess.approve")
        || approver.value(QStringLiteral("bindingHash")).type() != QVariant::String
        || approver.value(QStringLiteral("bindingHash")).toString() != bindingHash
        || !_clock.isValid() || _clock.elapsed() < 0 || _clock.elapsed() > 9007199254740991LL - 30000)
        return startAuthenticationDenied(QStringLiteral("ground-conditional-authentication-shape-invalid"));
    const qint64 originalBudgetDeadline = _clock.elapsed() + maximumRemainingMs;
    const auto actualOwnerCurrent = [&]() {
        return self && owner && owner->thread() == thread() && owner->approvalEpoch() == approvalEpoch
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())
            && _clock.elapsed() < originalBudgetDeadline
            && owner->authenticationConsumeContextCurrent(approvalId, approvalEpoch, bindingHash,
                reviewId, reviewEpoch, primary, approver, maximumRemainingMs)
            && self && owner && owner->thread() == thread() && owner->approvalEpoch() == approvalEpoch
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())
            && _clock.elapsed() < originalBudgetDeadline;
    };
    if (!actualOwnerCurrent() || !identitySnapshotCurrent(primary) || !self || !owner || !actualOwnerCurrent()
        || !approverSnapshotCurrent(approver) || !self || !owner || !actualOwnerCurrent())
        return startAuthenticationDenied(QStringLiteral("ground-conditional-authentication-source-invalid"));
    if (!_groundStartApprovalAuthentication.isEmpty()) {
        const QVariantMap previous = _groundStartApprovalAuthentication;
        const QPointer<FleetMissionGroundStartApproval> previousOwner(_groundStartApprovalOwner);
        const bool current = groundStartApprovalAuthenticationCurrent(previous, previousOwner.data(),
            previous.value(QStringLiteral("approvalId")).toString(), previous.value(QStringLiteral("approvalEpoch")).toString(),
            previous.value(QStringLiteral("bindingHash")).toString(), previous.value(QStringLiteral("reviewId")).toString(),
            previous.value(QStringLiteral("reviewEpoch")).toString());
        if (!self || !owner) return startAuthenticationDenied(QStringLiteral("ground-conditional-owner-destroyed"));
        if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data()))
            return startAuthenticationDenied(QStringLiteral("ground-conditional-episode-changed-during-validation"));
        if (_groundStartApprovalAuthentication != previous || _groundStartApprovalOwner != previousOwner)
            return startAuthenticationDenied(QStringLiteral("ground-conditional-replacement-changed"));
        if (current) return startAuthenticationDenied(QStringLiteral("ground-conditional-authentication-already-owned"));
        clearGroundStartApprovalAuthentication();
    }
    if (!actualOwnerCurrent() || !identitySnapshotCurrent(primary) || !self || !owner || !actualOwnerCurrent()
        || !approverSnapshotCurrent(approver) || !self || !owner || !actualOwnerCurrent())
        return startAuthenticationDenied(QStringLiteral("ground-conditional-source-changed-before-consumption"));
    const qint64 consumed = _clock.elapsed();
    const qint64 expires = qMin(originalBudgetDeadline, qMin(
        qint64(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
        qint64(approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())));
    if (expires <= consumed) return startAuthenticationDenied(QStringLiteral("ground-conditional-original-deadline-expired"));
    const QString authenticationId = uuid();
    const QVariantMap proof{{QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("approvalId"), approvalId},
        {QStringLiteral("approvalEpoch"), approvalEpoch}, {QStringLiteral("reviewId"), reviewId}, {QStringLiteral("reviewEpoch"), reviewEpoch},
        {QStringLiteral("authenticationId"), authenticationId}, {QStringLiteral("approverAuthenticationId"), approver.value(QStringLiteral("sessionId"))},
        {QStringLiteral("primarySessionId"), primary.value(QStringLiteral("sessionId"))},
        {QStringLiteral("primaryActorId"), primary.value(QStringLiteral("actorId"))}, {QStringLiteral("primaryRole"), primary.value(QStringLiteral("role"))},
        {QStringLiteral("approverActorId"), approver.value(QStringLiteral("actorId"))}, {QStringLiteral("approverRole"), approver.value(QStringLiteral("role"))},
        {QStringLiteral("sessionEpoch"), _sessionEpoch}, {QStringLiteral("sessionRevision"), QString::number(_sessionRevision)},
        {QStringLiteral("action"), QStringLiteral("mission.start.ground.reassess.approve")}, {QStringLiteral("bindingHash"), bindingHash},
        {QStringLiteral("purpose"), QStringLiteral("ground-reassess-conditional-authentication-not-permit")},
        {QStringLiteral("consumedAtMonotonicMs"), double(consumed)}, {QStringLiteral("expiresAtMonotonicMs"), double(expires)},
        {QStringLiteral("durationMs"), double(expires - consumed)}, {QStringLiteral("originalBudgetExpiresAtMonotonicMs"), double(originalBudgetDeadline)},
        {QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"), approver.value(QStringLiteral("issuedAtMonotonicMs")).toDouble()}};
    if (!groundAuthenticationShape(proof)) return startAuthenticationDenied(QStringLiteral("ground-conditional-proof-shape-invalid"));
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data()))
        return startAuthenticationDenied(QStringLiteral("ground-conditional-episode-changed-before-consumption"));
    // One irreversible pending-approver consumption. Old Start/Authority/Txn
    // records are untouched; this independent conditional basis cannot dispatch.
    _approverReceipt.clear(); _approvedApplicantIdentity.clear();
    _groundStartApprovalAuthentication = proof;
    _groundStartApprovalPrimaryIdentity = canonicalReceiptNumbers(primary);
    _groundStartApprovalApproverIdentity = canonicalReceiptNumbers(approver);
    _groundStartApprovalOwner = owner;
    _groundStartApprovalOwnerDestroyedConnection = connect(owner.data(), &QObject::destroyed, this,
        [this, approvalId, approvalEpoch, authenticationId]() {
            if (_groundStartApprovalAuthentication.value(QStringLiteral("approvalId")).toString() != approvalId
                || _groundStartApprovalAuthentication.value(QStringLiteral("approvalEpoch")).toString() != approvalEpoch
                || _groundStartApprovalAuthentication.value(QStringLiteral("authenticationId")).toString() != authenticationId) return;
            clearGroundStartApprovalAuthentication(); emit changed();
        });
    const bool stillOwned = actualOwnerCurrent();
    if (!self || !owner) return startAuthenticationDenied(QStringLiteral("ground-conditional-owner-destroyed-after-consumption"));
    if (!stillOwned || _groundStartApprovalAuthentication != proof || _groundStartApprovalOwner != owner
        || !groundStartApprovalAuthenticationCurrent(proof, owner.data(), approvalId, approvalEpoch, bindingHash, reviewId, reviewEpoch)
        || !self || !owner || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())) {
        if (self && _groundStartApprovalAuthentication == proof && _groundStartApprovalOwner == owner)
            clearGroundStartApprovalAuthentication();
        return startAuthenticationDenied(QStringLiteral("ground-conditional-source-changed-after-consumption"));
    }
    // No synchronous signal: the actual conditional ledger must commit first.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), proof},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::groundStartApprovalAuthenticationCurrent(const QVariantMap& suppliedReceipt, QObject* approvalObject,
    const QString& requestedApprovalId, const QString& requestedApprovalEpoch, const QString& requestedBindingHash,
    const QString& requestedReviewId, const QString& requestedReviewEpoch) const
{
    const QVariantMap proof = suppliedReceipt;
    const QString approvalId = requestedApprovalId, approvalEpoch = requestedApprovalEpoch;
    const QString bindingHash = requestedBindingHash, reviewId = requestedReviewId, reviewEpoch = requestedReviewEpoch;
    const QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QPointer<FleetMissionGroundStartApproval> owner(qobject_cast<FleetMissionGroundStartApproval*>(approvalObject));
    if (!ownerThread() || _promptActive || !owner || owner != _groundStartApprovalOwner || owner->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())
        || owner->approvalEpoch() != approvalEpoch || !groundAuthenticationShape(proof)
        || canonicalGroundAuthenticationNumbers(proof) != _groundStartApprovalAuthentication
        || proof.value(QStringLiteral("approvalId")).toString() != approvalId || proof.value(QStringLiteral("approvalEpoch")).toString() != approvalEpoch
        || proof.value(QStringLiteral("bindingHash")).toString() != bindingHash || proof.value(QStringLiteral("reviewId")).toString() != reviewId
        || proof.value(QStringLiteral("reviewEpoch")).toString() != reviewEpoch) return false;
    const QVariantMap primary = _groundStartApprovalPrimaryIdentity, approver = _groundStartApprovalApproverIdentity;
    if (!identitySnapshotCurrent(primary) || !self || !owner
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())) return false;
    const QString role = currentRole(proof.value(QStringLiteral("approverActorId")).toString());
    if (!self || !owner || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())
        || !workstationCurrent() || !self || !owner
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())) return false;
    return FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, owner.data())
        && !_promptActive && owner == _groundStartApprovalOwner && owner->thread() == thread() && owner->approvalEpoch() == approvalEpoch
        && canonicalGroundAuthenticationNumbers(proof) == _groundStartApprovalAuthentication
        && primary == _groundStartApprovalPrimaryIdentity && approver == _groundStartApprovalApproverIdentity && primary == _identityReceipt
        && proof.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && proof.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && proof.value(QStringLiteral("primarySessionId")) == primary.value(QStringLiteral("sessionId"))
        && proof.value(QStringLiteral("primaryActorId")) == primary.value(QStringLiteral("actorId"))
        && proof.value(QStringLiteral("primaryRole")) == primary.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("primaryActorId")) != proof.value(QStringLiteral("approverActorId"))
        && proof.value(QStringLiteral("approverActorId")) == approver.value(QStringLiteral("actorId"))
        && proof.value(QStringLiteral("approverAuthenticationId")) == approver.value(QStringLiteral("sessionId"))
        && approverRole(role) && role == proof.value(QStringLiteral("approverRole")).toString()
        && proof.value(QStringLiteral("approverRole")) == approver.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("action")) == approver.value(QStringLiteral("action"))
        && proof.value(QStringLiteral("bindingHash")) == approver.value(QStringLiteral("bindingHash"))
        && proof.value(QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs")) == approver.value(QStringLiteral("issuedAtMonotonicMs"))
        && _clock.elapsed() >= proof.value(QStringLiteral("consumedAtMonotonicMs")).toDouble()
        && _clock.elapsed() < proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble() == qMin(
            proof.value(QStringLiteral("originalBudgetExpiresAtMonotonicMs")).toDouble(), qMin(
                primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()));
}

bool FleetOperatorSession::revokeGroundStartApprovalAuthentication(QObject* approvalObject, const QString& requestedApprovalId,
    const QString& requestedApprovalEpoch)
{
    const QString approvalId = requestedApprovalId, approvalEpoch = requestedApprovalEpoch;
    const QPointer<FleetMissionGroundStartApproval> owner(qobject_cast<FleetMissionGroundStartApproval*>(approvalObject));
    if (!ownerThread() || !owner || owner != _groundStartApprovalOwner || owner->approvalEpoch() != approvalEpoch
        || _groundStartApprovalAuthentication.value(QStringLiteral("approvalId")).toString() != approvalId
        || _groundStartApprovalAuthentication.value(QStringLiteral("approvalEpoch")).toString() != approvalEpoch) return false;
    clearGroundStartApprovalAuthentication();
    return true;
}

void FleetOperatorSession::clearGroundStartupAuthentication()
{
    QObject::disconnect(_groundStartupOwnerDestroyedConnection);
    _groundStartupOwnerDestroyedConnection = {};
    _groundStartupAuthentication.clear(); _groundStartupOriginalBasis.clear();
    _groundStartupPrimaryIdentity.clear(); _groundStartupApproverIdentity.clear(); _groundStartupOwner.clear();
}

QVariantMap FleetOperatorSession::transferGroundStartApprovalAuthentication(const QVariantMap& suppliedPrimary,
    const QVariantMap& suppliedBasis, QObject* approvalObject, QObject* authorityObject,
    const QString& suppliedStartupId, const QString& suppliedAuthorityEpoch, const QString& suppliedSourceHash,
    int maximumRemainingMs)
{
    const QVariantMap primary = suppliedPrimary, basis = suppliedBasis;
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetMissionGroundStartApproval> approval(qobject_cast<FleetMissionGroundStartApproval*>(approvalObject));
    const QPointer<FleetMissionGroundStartupAuthority> authority(qobject_cast<FleetMissionGroundStartupAuthority*>(authorityObject));
    if (!ownerThread() || _promptActive || !approval || !authority || approval->thread() != thread() || authority->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, approval.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || authority->authorityEpoch() != authorityEpoch || maximumRemainingMs < 1 || maximumRemainingMs > 30000
        || QUuid(startupId).isNull() || QUuid(startupId).toString(QUuid::WithoutBraces) != startupId
        || QUuid(authorityEpoch).isNull() || QUuid(authorityEpoch).toString(QUuid::WithoutBraces) != authorityEpoch
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(sourceHash).hasMatch() || !groundAuthenticationShape(basis))
        return startAuthenticationDenied(QStringLiteral("ground-startup-actual-owner-or-scope-invalid"));
    // Native clocks are not compared across owners. Fix this caller's already
    // shortened remaining budget before the first WTS/SAM query can reenter.
    const qint64 transferBudgetDeadline = _clock.elapsed() + maximumRemainingMs;
    const QString approvalId = basis.value(QStringLiteral("approvalId")).toString();
    const QString approvalEpoch = basis.value(QStringLiteral("approvalEpoch")).toString();
    const QString bindingHash = basis.value(QStringLiteral("bindingHash")).toString();
    const QString reviewId = basis.value(QStringLiteral("reviewId")).toString(), reviewEpoch = basis.value(QStringLiteral("reviewEpoch")).toString();
    const auto ownerCurrent = [&]() {
        return self && approval && authority && authority->thread() == thread() && authority->authorityEpoch() == authorityEpoch
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, approval.data())
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
            && approval->thread() == thread() && approval->approvalEpoch() == approvalEpoch && _clock.elapsed() < transferBudgetDeadline
            && authority->authenticationTransferContextCurrent(startupId, sourceHash, approval.data(), basis)
            && self && approval && authority && authority->thread() == thread() && authority->authorityEpoch() == authorityEpoch
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, approval.data())
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
            && approval->thread() == thread() && approval->approvalEpoch() == approvalEpoch && _clock.elapsed() < transferBudgetDeadline;
    };
    if (!ownerCurrent() || !identitySnapshotCurrent(primary) || !self || !approval || !authority || !ownerCurrent()
        || !groundStartApprovalAuthenticationCurrent(basis, approval.data(), approvalId, approvalEpoch, bindingHash, reviewId, reviewEpoch)
        || !self || !approval || !authority || !ownerCurrent())
        return startAuthenticationDenied(QStringLiteral("ground-startup-original-authentication-not-current"));
    if (!_groundStartupAuthentication.isEmpty()) {
        const QVariantMap previous = _groundStartupAuthentication;
        const QPointer<FleetMissionGroundStartupAuthority> previousOwner(_groundStartupOwner);
        const bool current = groundStartupAuthenticationCurrent(previous, previousOwner.data(),
            previous.value(QStringLiteral("startupId")).toString(), previous.value(QStringLiteral("authorityEpoch")).toString(),
            previous.value(QStringLiteral("sourceHash")).toString());
        if (!self || !approval || !authority) return startAuthenticationDenied(QStringLiteral("ground-startup-owner-destroyed"));
        if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, approval.data())
            || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data()))
            return startAuthenticationDenied(QStringLiteral("ground-startup-episode-changed-during-validation"));
        if (_groundStartupAuthentication != previous || _groundStartupOwner != previousOwner)
            return startAuthenticationDenied(QStringLiteral("ground-startup-replacement-changed"));
        if (current) return startAuthenticationDenied(QStringLiteral("ground-startup-authentication-already-owned"));
        clearGroundStartupAuthentication();
    }
    if (!ownerCurrent() || !identitySnapshotCurrent(primary) || !self || !approval || !authority || !ownerCurrent()
        || !groundStartApprovalAuthenticationCurrent(basis, approval.data(), approvalId, approvalEpoch, bindingHash, reviewId, reviewEpoch)
        || !self || !approval || !authority || !ownerCurrent())
        return startAuthenticationDenied(QStringLiteral("ground-startup-source-changed-before-move"));
    const QVariantMap original = _groundStartApprovalAuthentication;
    const QVariantMap primaryBasis = _groundStartApprovalPrimaryIdentity, approverBasis = _groundStartApprovalApproverIdentity;
    if (canonicalGroundAuthenticationNumbers(basis) != original || primaryBasis != canonicalReceiptNumbers(primary)
        || _groundStartApprovalOwner != approval)
        return startAuthenticationDenied(QStringLiteral("ground-startup-exact-basis-or-primary-mismatch"));
    const qint64 transferred = _clock.elapsed();
    const qint64 expires = qMin(transferBudgetDeadline, qMin(qint64(original.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
        qMin(qint64(primaryBasis.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
            qint64(approverBasis.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()))));
    if (expires <= transferred) return startAuthenticationDenied(QStringLiteral("ground-startup-original-budget-expired"));
    const QString authorityAuthenticationId = uuid();
    QVariantMap proof = original;
    proof[QStringLiteral("purpose")] = QStringLiteral("ground-startup-authentication-not-permit");
    proof[QStringLiteral("expiresAtMonotonicMs")] = double(expires);
    proof[QStringLiteral("durationMs")] = double(expires - qint64(original.value(QStringLiteral("consumedAtMonotonicMs")).toDouble()));
    proof.insert(QStringLiteral("startupId"), startupId); proof.insert(QStringLiteral("authorityEpoch"), authorityEpoch);
    proof.insert(QStringLiteral("sourceHash"), sourceHash); proof.insert(QStringLiteral("authorityAuthenticationId"), authorityAuthenticationId);
    proof.insert(QStringLiteral("transferredAtMonotonicMs"), double(transferred));
    proof.insert(QStringLiteral("transferBudgetExpiresAtMonotonicMs"), double(transferBudgetDeadline));
    if (!groundStartupAuthenticationShape(proof)) return startAuthenticationDenied(QStringLiteral("ground-startup-proof-shape-invalid"));
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, approval.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data()))
        return startAuthenticationDenied(QStringLiteral("ground-startup-episode-changed-before-move"));
    // One-way move, not a new login or session epoch. Disconnect the old owner
    // so its later Review invalidation/destruction cannot clear the new basis.
    clearGroundStartApprovalAuthentication();
    _groundStartupAuthentication = proof; _groundStartupOriginalBasis = original;
    _groundStartupPrimaryIdentity = primaryBasis; _groundStartupApproverIdentity = approverBasis; _groundStartupOwner = authority;
    _groundStartupOwnerDestroyedConnection = connect(authority.data(), &QObject::destroyed, this,
        [this, startupId, authorityEpoch, authorityAuthenticationId]() {
            if (_groundStartupAuthentication.value(QStringLiteral("startupId")).toString() != startupId
                || _groundStartupAuthentication.value(QStringLiteral("authorityEpoch")).toString() != authorityEpoch
                || _groundStartupAuthentication.value(QStringLiteral("authorityAuthenticationId")).toString() != authorityAuthenticationId) return;
            clearGroundStartupAuthentication(); emit changed();
        });
    const bool retainedContext = ownerCurrent();
    if (!self || !authority) return startAuthenticationDenied(QStringLiteral("ground-startup-owner-destroyed-after-move"));
    if (!retainedContext || _groundStartupAuthentication != proof || _groundStartupOwner != authority
        || !groundStartupAuthenticationCurrent(proof, authority.data(), startupId, authorityEpoch, sourceHash) || !self || !authority
        || !approval || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, approval.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())) {
        if (self && _groundStartupAuthentication == proof && _groundStartupOwner == authority) clearGroundStartupAuthentication();
        return startAuthenticationDenied(QStringLiteral("ground-startup-source-changed-after-move"));
    }
    const qint64 remaining = expires - _clock.elapsed();
    if (remaining < 1 || remaining > 30000) {
        if (_groundStartupAuthentication == proof && _groundStartupOwner == authority) clearGroundStartupAuthentication();
        return startAuthenticationDenied(QStringLiteral("ground-startup-original-budget-expired-at-return"));
    }
    // remaining is calculated AFTER every native query. duration is historical
    // expiry-oldConsumedAt and MUST NOT be reused as a renewed owner budget.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), proof}, {QStringLiteral("remainingMs"), double(remaining)},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::groundStartupAuthenticationCurrent(const QVariantMap& suppliedReceipt, QObject* authorityObject,
    const QString& suppliedStartupId, const QString& suppliedAuthorityEpoch, const QString& suppliedSourceHash) const
{
    const QVariantMap proof = suppliedReceipt;
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QPointer<FleetMissionGroundStartupAuthority> authority(qobject_cast<FleetMissionGroundStartupAuthority*>(authorityObject));
    if (!ownerThread() || _promptActive || !authority || authority != _groundStartupOwner || authority->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || authority->authorityEpoch() != authorityEpoch || !groundStartupAuthenticationShape(proof)
        || canonicalGroundStartupNumbers(proof) != _groundStartupAuthentication
        || proof.value(QStringLiteral("startupId")).toString() != startupId || proof.value(QStringLiteral("authorityEpoch")).toString() != authorityEpoch
        || proof.value(QStringLiteral("sourceHash")).toString() != sourceHash) return false;
    const QVariantMap original = _groundStartupOriginalBasis, primary = _groundStartupPrimaryIdentity, approver = _groundStartupApproverIdentity;
    if (!groundAuthenticationShape(original) || !identitySnapshotCurrent(primary) || !self || !authority
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())) return false;
    const QString role = currentRole(proof.value(QStringLiteral("approverActorId")).toString());
    if (!self || !authority || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !workstationCurrent() || !self || !authority
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())) return false;
    QVariantMap shortened = original;
    shortened[QStringLiteral("expiresAtMonotonicMs")] = proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    shortened[QStringLiteral("durationMs")] = proof.value(QStringLiteral("durationMs")).toDouble();
    shortened[QStringLiteral("purpose")] = QStringLiteral("ground-startup-authentication-not-permit");
    for (const QString& key : {QStringLiteral("startupId"), QStringLiteral("authorityEpoch"), QStringLiteral("sourceHash"),
        QStringLiteral("authorityAuthenticationId"), QStringLiteral("transferredAtMonotonicMs"), QStringLiteral("transferBudgetExpiresAtMonotonicMs")})
        shortened.insert(key, proof.value(key));
    return FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        && !_promptActive && authority == _groundStartupOwner && authority->thread() == thread() && authority->authorityEpoch() == authorityEpoch
        && canonicalGroundStartupNumbers(proof) == _groundStartupAuthentication && canonicalGroundStartupNumbers(proof) == shortened
        && original == _groundStartupOriginalBasis && primary == _groundStartupPrimaryIdentity && approver == _groundStartupApproverIdentity
        && primary == _identityReceipt && proof.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && proof.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && proof.value(QStringLiteral("primarySessionId")) == primary.value(QStringLiteral("sessionId"))
        && proof.value(QStringLiteral("primaryActorId")) == primary.value(QStringLiteral("actorId"))
        && proof.value(QStringLiteral("primaryRole")) == primary.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("primaryActorId")) != proof.value(QStringLiteral("approverActorId"))
        && proof.value(QStringLiteral("approverActorId")) == approver.value(QStringLiteral("actorId"))
        && proof.value(QStringLiteral("approverAuthenticationId")) == approver.value(QStringLiteral("sessionId"))
        && approverRole(role) && role == proof.value(QStringLiteral("approverRole")).toString()
        && proof.value(QStringLiteral("approverRole")) == approver.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("action")) == approver.value(QStringLiteral("action"))
        && proof.value(QStringLiteral("bindingHash")) == approver.value(QStringLiteral("bindingHash"))
        && proof.value(QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs")) == approver.value(QStringLiteral("issuedAtMonotonicMs"))
        && _clock.elapsed() >= proof.value(QStringLiteral("transferredAtMonotonicMs")).toDouble()
        && _clock.elapsed() < proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble() == qMin(proof.value(QStringLiteral("transferBudgetExpiresAtMonotonicMs")).toDouble(),
            qMin(original.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), qMin(
                primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())));
}

bool FleetOperatorSession::revokeGroundStartupAuthentication(QObject* authorityObject, const QString& suppliedStartupId,
    const QString& suppliedAuthorityEpoch)
{
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch;
    const QPointer<FleetMissionGroundStartupAuthority> authority(qobject_cast<FleetMissionGroundStartupAuthority*>(authorityObject));
    if (!ownerThread() || !authority || authority != _groundStartupOwner || authority->authorityEpoch() != authorityEpoch
        || _groundStartupAuthentication.value(QStringLiteral("startupId")).toString() != startupId
        || _groundStartupAuthentication.value(QStringLiteral("authorityEpoch")).toString() != authorityEpoch) return false;
    clearGroundStartupAuthentication(); return true;
}

void FleetOperatorSession::clearGroundStartupTransactionAuthentication()
{
    QObject::disconnect(_groundStartupTransactionOwnerDestroyedConnection);
    _groundStartupTransactionOwnerDestroyedConnection = {};
    _groundStartupTransactionAuthentication.clear();
    _groundStartupTransactionOriginalAuthorityBasis.clear(); _groundStartupTransactionOriginalApprovalBasis.clear();
    _groundStartupTransactionPrimaryIdentity.clear(); _groundStartupTransactionApproverIdentity.clear();
    _groundStartupTransactionOwner.clear();
    _groundStartupTransactionTransferBudgetDeadlineMs = -1; _groundStartupTransactionTransferredAtMs = -1;
}

QVariantMap FleetOperatorSession::transferGroundStartupAuthenticationToTransaction(const QVariantMap& suppliedAuthentication,
    QObject* authorityObject, QObject* transactionObject, const QString& suppliedStartupId,
    const QString& suppliedAuthorityEpoch, const QString& suppliedTransactionEpoch, const QString& suppliedSourceHash,
    int maximumRemainingMs)
{
    // Never retain references into a caller that OS callbacks may delete.
    const QVariantMap authentication = suppliedAuthentication;
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch;
    const QString transactionEpoch = suppliedTransactionEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetMissionGroundStartupAuthority> authority(qobject_cast<FleetMissionGroundStartupAuthority*>(authorityObject));
    const QPointer<FleetMissionStartupTransaction> transaction(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    if (!ownerThread() || _promptActive || !authority || !transaction || authority->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || transaction->thread() != thread() || authority->authorityEpoch() != authorityEpoch
        || transaction->transactionEpoch() != transactionEpoch || maximumRemainingMs < 1 || maximumRemainingMs > 30000
        || !groundStartupAuthenticationShape(authentication)
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(sourceHash).hasMatch())
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-actual-owner-or-scope-invalid"));
    for (const QString& value : {startupId, authorityEpoch, transactionEpoch})
        if (QUuid(value).isNull() || QUuid(value).toString(QUuid::WithoutBraces) != value)
            return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-exact-scope-required"));
    // Fix the already-shortened caller budget BEFORE any WTS/SAM query. The
    // original 28-field timestamps stay historical, not a renewed lifetime.
    const qint64 transferBudgetDeadline = _clock.elapsed() + maximumRemainingMs;
    const QVariantMap authorityContext = authority->pendingStartupTransactionContext(transaction.data(), startupId, transactionEpoch, sourceHash);
    if (!self || !authority || !transaction || authorityContext.size() != 27
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || authorityContext.value(QStringLiteral("ok")).type() != QVariant::Bool || !authorityContext.value(QStringLiteral("ok")).toBool()
        || authorityContext.value(QStringLiteral("startupId")).toString() != startupId
        || authorityContext.value(QStringLiteral("authorityEpoch")).toString() != authorityEpoch
        || authorityContext.value(QStringLiteral("sourceHash")).toString() != sourceHash)
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-native-pending-context-required"));
    const auto ownerCurrent = [&]() {
        return self && authority && transaction && authority->thread() == thread() && transaction->thread() == thread()
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
            && authority->authorityEpoch() == authorityEpoch && transaction->transactionEpoch() == transactionEpoch
            && _clock.elapsed() < transferBudgetDeadline
            && authority->pendingStartupTransactionContext(transaction.data(), startupId, transactionEpoch, sourceHash) == authorityContext
            && transaction->authenticationTransferContextCurrent(startupId, sourceHash, authority.data(), authorityContext)
            && self && authority && transaction && authority->thread() == thread() && transaction->thread() == thread()
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
            && FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
            && authority->authorityEpoch() == authorityEpoch && transaction->transactionEpoch() == transactionEpoch
            && _clock.elapsed() < transferBudgetDeadline
            && authority->pendingStartupTransactionContext(transaction.data(), startupId, transactionEpoch, sourceHash) == authorityContext;
    };
    if (!ownerCurrent() || !groundStartupAuthenticationCurrent(authentication, authority.data(), startupId, authorityEpoch, sourceHash)
        || !self || !authority || !transaction || !ownerCurrent())
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-original-authentication-not-current"));
    if (!_groundStartupTransactionAuthentication.isEmpty()) {
        const QVariantMap previous = _groundStartupTransactionAuthentication;
        const QPointer<FleetMissionStartupTransaction> previousOwner(_groundStartupTransactionOwner);
        const bool current = groundStartupTransactionAuthenticationCurrent(previous, previousOwner.data(),
            previous.value(QStringLiteral("startupId")).toString(), previous.value(QStringLiteral("transactionEpoch")).toString(),
            previous.value(QStringLiteral("sourceHash")).toString());
        if (!self || !authority || !transaction) return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-owner-destroyed"));
        if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
            || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data()))
            return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-episode-changed-during-validation"));
        if (_promptActive) return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-prompt-busy"));
        if (_groundStartupTransactionAuthentication != previous || _groundStartupTransactionOwner != previousOwner)
            return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-replacement-changed"));
        if (current) return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-authentication-already-owned"));
        clearGroundStartupTransactionAuthentication();
    }
    if (!ownerCurrent() || !groundStartupAuthenticationCurrent(authentication, authority.data(), startupId, authorityEpoch, sourceHash)
        || !self || !authority || !transaction || !ownerCurrent())
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-source-changed-before-move"));
    const QVariantMap originalAuthority = _groundStartupAuthentication, originalApproval = _groundStartupOriginalBasis;
    const QVariantMap primary = _groundStartupPrimaryIdentity, approver = _groundStartupApproverIdentity;
    if (_groundStartupOwner != authority || canonicalGroundStartupNumbers(authentication) != originalAuthority
        || !groundStartupOriginalBasisMatches(originalAuthority, originalApproval))
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-exact-original-basis-required"));
    const qint64 transferred = _clock.elapsed();
    const qint64 expires = qMin(transferBudgetDeadline, qMin(qint64(originalAuthority.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
        qMin(qint64(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
            qint64(approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()))));
    if (expires <= transferred) return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-original-budget-expired"));
    const QString transactionAuthenticationId = uuid();
    QVariantMap proof = originalAuthority;
    proof[QStringLiteral("purpose")] = QStringLiteral("ground-startup-transaction-authentication-not-permit");
    proof[QStringLiteral("expiresAtMonotonicMs")] = double(expires);
    proof[QStringLiteral("durationMs")] = double(expires - qint64(originalAuthority.value(QStringLiteral("consumedAtMonotonicMs")).toDouble()));
    proof.insert(QStringLiteral("transactionEpoch"), transactionEpoch);
    proof.insert(QStringLiteral("transactionAuthenticationId"), transactionAuthenticationId);
    if (!groundStartupTransactionAuthenticationShape(proof))
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-proof-shape-invalid"));
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data()))
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-episode-changed-before-move"));
    // Irreversible retirement of ground Authority proof ONLY; old non-ground
    // transaction proof24 and its original owner/nonce are separate storage.
    clearGroundStartupAuthentication();
    _groundStartupTransactionAuthentication = proof;
    _groundStartupTransactionOriginalAuthorityBasis = originalAuthority; _groundStartupTransactionOriginalApprovalBasis = originalApproval;
    _groundStartupTransactionPrimaryIdentity = primary; _groundStartupTransactionApproverIdentity = approver;
    _groundStartupTransactionOwner = transaction;
    _groundStartupTransactionTransferBudgetDeadlineMs = transferBudgetDeadline; _groundStartupTransactionTransferredAtMs = transferred;
    _groundStartupTransactionOwnerDestroyedConnection = connect(transaction.data(), &QObject::destroyed, this,
        [this, startupId, transactionEpoch, transactionAuthenticationId]() {
            if (_groundStartupTransactionAuthentication.value(QStringLiteral("startupId")).toString() != startupId
                || _groundStartupTransactionAuthentication.value(QStringLiteral("transactionEpoch")).toString() != transactionEpoch
                || _groundStartupTransactionAuthentication.value(QStringLiteral("transactionAuthenticationId")).toString() != transactionAuthenticationId) return;
            clearGroundStartupTransactionAuthentication(); emit changed();
        });
    const bool retainedContext = ownerCurrent();
    if (!self || !authority || !transaction) {
        if (self && _groundStartupTransactionAuthentication == proof && _groundStartupTransactionOwner == transaction)
            clearGroundStartupTransactionAuthentication();
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-owner-destroyed-after-move"));
    }
    if (!retainedContext || _groundStartupTransactionAuthentication != proof || _groundStartupTransactionOwner != transaction
        || !groundStartupTransactionAuthenticationCurrent(proof, transaction.data(), startupId, transactionEpoch, sourceHash)
        || !self || !authority || !transaction || !ownerCurrent()) {
        if (self && _groundStartupTransactionAuthentication == proof && _groundStartupTransactionOwner == transaction)
            clearGroundStartupTransactionAuthentication();
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-source-changed-after-move"));
    }
    const qint64 remaining = expires - _clock.elapsed();
    if (remaining < 1 || remaining > 30000) {
        if (_groundStartupTransactionAuthentication == proof && _groundStartupTransactionOwner == transaction)
            clearGroundStartupTransactionAuthentication();
        return startAuthenticationDenied(QStringLiteral("ground-startup-transaction-original-budget-expired-at-return"));
    }
    // No changed signal or native transport here. The consumer commits before
    // publication; remainingMs includes ALL elapsed OS/context query time.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), proof}, {QStringLiteral("remainingMs"), double(remaining)},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::groundStartupTransactionAuthenticationCurrent(const QVariantMap& suppliedReceipt,
    QObject* transactionObject, const QString& suppliedStartupId, const QString& suppliedTransactionEpoch,
    const QString& suppliedSourceHash) const
{
    const QVariantMap proof = suppliedReceipt;
    const QString startupId = suppliedStartupId, transactionEpoch = suppliedTransactionEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QPointer<FleetMissionStartupTransaction> transaction(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    if (!ownerThread() || _promptActive || !transaction || transaction != _groundStartupTransactionOwner
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || transaction->thread() != thread() || transaction->transactionEpoch() != transactionEpoch
        || !groundStartupTransactionAuthenticationShape(proof) || canonicalGroundStartupNumbers(proof) != _groundStartupTransactionAuthentication
        || proof.value(QStringLiteral("startupId")).toString() != startupId
        || proof.value(QStringLiteral("transactionEpoch")).toString() != transactionEpoch
        || proof.value(QStringLiteral("sourceHash")).toString() != sourceHash) return false;
    const QVariantMap originalAuthority = _groundStartupTransactionOriginalAuthorityBasis, originalApproval = _groundStartupTransactionOriginalApprovalBasis;
    const QVariantMap primary = _groundStartupTransactionPrimaryIdentity, approver = _groundStartupTransactionApproverIdentity;
    const qint64 budgetDeadline = _groundStartupTransactionTransferBudgetDeadlineMs, transferred = _groundStartupTransactionTransferredAtMs;
    if (transferred < 0 || budgetDeadline <= transferred || budgetDeadline - transferred > 30000
        || !groundStartupOriginalBasisMatches(originalAuthority, originalApproval)
        || !identitySnapshotCurrent(primary) || !self || !transaction
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())) return false;
    const QString role = currentRole(proof.value(QStringLiteral("approverActorId")).toString());
    if (!self || !transaction || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || !workstationCurrent() || !self || !transaction
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())) return false;
    QVariantMap expected = canonicalGroundStartupNumbers(originalAuthority);
    expected[QStringLiteral("expiresAtMonotonicMs")] = proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
    expected[QStringLiteral("durationMs")] = proof.value(QStringLiteral("durationMs")).toDouble();
    expected[QStringLiteral("purpose")] = QStringLiteral("ground-startup-transaction-authentication-not-permit");
    expected.insert(QStringLiteral("transactionEpoch"), transactionEpoch);
    expected.insert(QStringLiteral("transactionAuthenticationId"), proof.value(QStringLiteral("transactionAuthenticationId")));
    return FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        && !_promptActive && transaction == _groundStartupTransactionOwner && transaction->thread() == thread()
        && transaction->transactionEpoch() == transactionEpoch && canonicalGroundStartupNumbers(proof) == _groundStartupTransactionAuthentication
        && canonicalGroundStartupNumbers(proof) == expected
        && originalAuthority == _groundStartupTransactionOriginalAuthorityBasis && originalApproval == _groundStartupTransactionOriginalApprovalBasis
        && primary == _groundStartupTransactionPrimaryIdentity && approver == _groundStartupTransactionApproverIdentity
        && budgetDeadline == _groundStartupTransactionTransferBudgetDeadlineMs && transferred == _groundStartupTransactionTransferredAtMs
        && primary == _identityReceipt && proof.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && proof.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && proof.value(QStringLiteral("primarySessionId")) == primary.value(QStringLiteral("sessionId"))
        && proof.value(QStringLiteral("primaryActorId")) == primary.value(QStringLiteral("actorId"))
        && proof.value(QStringLiteral("primaryRole")) == primary.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("primaryActorId")) != proof.value(QStringLiteral("approverActorId"))
        && proof.value(QStringLiteral("approverActorId")) == approver.value(QStringLiteral("actorId"))
        && proof.value(QStringLiteral("approverAuthenticationId")) == approver.value(QStringLiteral("sessionId"))
        && approverRole(role) && role == proof.value(QStringLiteral("approverRole")).toString()
        && proof.value(QStringLiteral("approverRole")) == approver.value(QStringLiteral("role"))
        && proof.value(QStringLiteral("action")) == approver.value(QStringLiteral("action"))
        && proof.value(QStringLiteral("bindingHash")) == approver.value(QStringLiteral("bindingHash"))
        && proof.value(QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs")) == approver.value(QStringLiteral("issuedAtMonotonicMs"))
        && transferred >= originalAuthority.value(QStringLiteral("transferredAtMonotonicMs")).toDouble()
        && _clock.elapsed() >= transferred && _clock.elapsed() < proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && originalAuthority.value(QStringLiteral("expiresAtMonotonicMs")).toDouble() == qMin(
            originalAuthority.value(QStringLiteral("transferBudgetExpiresAtMonotonicMs")).toDouble(), qMin(
                originalApproval.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), qMin(
                    primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())))
        && proof.value(QStringLiteral("expiresAtMonotonicMs")).toDouble() == qMin(double(budgetDeadline), qMin(
            originalAuthority.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), qMin(
                primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())));
}

bool FleetOperatorSession::revokeGroundStartupTransactionAuthentication(QObject* transactionObject,
    const QString& suppliedStartupId, const QString& suppliedTransactionEpoch)
{
    const QString startupId = suppliedStartupId, transactionEpoch = suppliedTransactionEpoch;
    const QPointer<FleetMissionStartupTransaction> transaction(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    if (!ownerThread() || !transaction || transaction != _groundStartupTransactionOwner || transaction->thread() != thread()
        || transaction->transactionEpoch() != transactionEpoch
        || _groundStartupTransactionAuthentication.value(QStringLiteral("startupId")).toString() != startupId
        || _groundStartupTransactionAuthentication.value(QStringLiteral("transactionEpoch")).toString() != transactionEpoch) return false;
    clearGroundStartupTransactionAuthentication(); return true;
}

void FleetOperatorSession::clearStartupAuthentication()
{
    QObject::disconnect(_startupOwnerDestroyedConnection);
    _startupOwnerDestroyedConnection = {};
    _startupAuthentication.clear();
    _startupPrimaryIdentity.clear();
    _startupApproverIdentity.clear();
    _startupAuthenticationOwner.clear();
}

QVariantMap FleetOperatorSession::transferStartApprovalAuthentication(const QVariantMap& suppliedPrimary,
    const QVariantMap& suppliedAuthentication, QObject* authorityObject, const QString& suppliedStartupId,
    const QString& suppliedAuthorityEpoch, const QString& suppliedSourceHash, int maximumRemainingMs)
{
    const QVariantMap primary = suppliedPrimary, authentication = suppliedAuthentication;
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetMissionStartupAuthority> authority(qobject_cast<FleetMissionStartupAuthority*>(authorityObject));
    if (!ownerThread() || _promptActive || maximumRemainingMs < 1 || maximumRemainingMs > 30000
        || !authority || authority->thread() != thread() || authority->authorityEpoch() != authorityEpoch
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || QUuid(startupId).isNull() || QUuid(startupId).toString(QUuid::WithoutBraces) != startupId
        || QUuid(authorityEpoch).isNull() || QUuid(authorityEpoch).toString(QUuid::WithoutBraces) != authorityEpoch
        || !QRegularExpression(QStringLiteral("^[0-9a-f]{64}$")).match(sourceHash).hasMatch())
        return startAuthenticationDenied(QStringLiteral("startup-authentication-owner-or-scope-invalid"));
    // Budget is fixed BEFORE any OS/native validation; validation consumes it,
    // never starts a fresh review window at the end of the transfer call.
    const qint64 budgetDeadline = _clock.elapsed() + maximumRemainingMs;
    if (!identitySnapshotCurrent(primary) || !self || !authority
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !startApprovalAuthenticationCurrent(authentication) || !self || !authority
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data()))
        return startAuthenticationDenied(QStringLiteral("startup-authentication-source-invalid"));
    const QVariantMap primaryBasis = _startApprovalPrimaryIdentity, approverBasis = _startApprovalApproverIdentity;
    if (!_startupAuthentication.isEmpty()) {
        const QVariantMap previous = _startupAuthentication;
        const QPointer<FleetMissionStartupAuthority> previousOwner(_startupAuthenticationOwner);
        const bool previousCurrent = startupAuthenticationCurrent(previous, previousOwner.data(),
            previous.value(QStringLiteral("startupId")).toString(), previous.value(QStringLiteral("authorityEpoch")).toString(),
            previous.value(QStringLiteral("sourceHash")).toString());
        if (!self) return startAuthenticationDenied(QStringLiteral("session-destroyed"));
        if (!authority || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data()))
            return startAuthenticationDenied(QStringLiteral("startup-authentication-episode-changed-during-validation"));
        if (_startupAuthentication != previous || _startupAuthenticationOwner != previousOwner)
            return startAuthenticationDenied(QStringLiteral("startup-authentication-owner-changed-during-validation"));
        if (previousCurrent) return startAuthenticationDenied(QStringLiteral("startup-authentication-already-owned"));
        clearStartupAuthentication();
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !identitySnapshotCurrent(primary) || !self || !authority
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !startApprovalAuthenticationCurrent(authentication)
        || !self || !authority || authority->thread() != thread() || authority->authorityEpoch() != authorityEpoch
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || primaryBasis != _startApprovalPrimaryIdentity || approverBasis != _startApprovalApproverIdentity)
        return startAuthenticationDenied(QStringLiteral("startup-authentication-source-changed"));
    const qint64 transferred = _clock.elapsed();
    const qint64 originalDeadline = qMin(qint64(primaryBasis.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()),
                                        qint64(approverBasis.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()));
    const qint64 expires = qMin(budgetDeadline, qMin(originalDeadline,
        qint64(authentication.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())));
    if (expires <= transferred) return startAuthenticationDenied(QStringLiteral("startup-authentication-original-budget-expired"));
    const QString authenticationId = uuid();
    const QVariantMap receipt {{QStringLiteral("schemaVersion"), 1.0}, {QStringLiteral("startupId"), startupId},
        {QStringLiteral("authorityEpoch"), authorityEpoch}, {QStringLiteral("authenticationId"), authenticationId},
        {QStringLiteral("sourceHash"), sourceHash}, {QStringLiteral("approvalId"), authentication.value(QStringLiteral("approvalId"))},
        {QStringLiteral("approverAuthenticationId"), authentication.value(QStringLiteral("approverAuthenticationId"))},
        {QStringLiteral("primarySessionId"), primaryBasis.value(QStringLiteral("sessionId"))},
        {QStringLiteral("sessionEpoch"), _sessionEpoch}, {QStringLiteral("sessionRevision"), QString::number(_sessionRevision)},
        {QStringLiteral("primaryActorId"), primaryBasis.value(QStringLiteral("actorId"))},
        {QStringLiteral("primaryRole"), primaryBasis.value(QStringLiteral("role"))},
        {QStringLiteral("approverActorId"), approverBasis.value(QStringLiteral("actorId"))},
        {QStringLiteral("approverRole"), approverBasis.value(QStringLiteral("role"))},
        {QStringLiteral("action"), QStringLiteral("mission.start")}, {QStringLiteral("bindingHash"), authentication.value(QStringLiteral("bindingHash"))},
        {QStringLiteral("transferredAtMonotonicMs"), double(transferred)}, {QStringLiteral("expiresAtMonotonicMs"), double(expires)},
        {QStringLiteral("durationMs"), double(expires - transferred)},
        {QStringLiteral("approvalAuthenticationWindowStartedAtMonotonicMs"), approverBasis.value(QStringLiteral("issuedAtMonotonicMs")).toDouble()},
        {QStringLiteral("originalAuthenticationExpiresAtMonotonicMs"), double(originalDeadline)},
        {QStringLiteral("purpose"), QStringLiteral("startup-authentication-basis-not-permit")}};
    if (!startupAuthenticationShape(receipt)) return startAuthenticationDenied(QStringLiteral("startup-authentication-shape-invalid"));
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data()))
        return startAuthenticationDenied(QStringLiteral("startup-authentication-episode-changed-before-move"));
    clearStartApprovalAuthentication(); // One-way retirement, NOT advanceSession.
    _startupAuthentication = receipt;
    _startupPrimaryIdentity = primaryBasis;
    _startupApproverIdentity = approverBasis;
    _startupAuthenticationOwner = authority;
    _startupOwnerDestroyedConnection = connect(authority.data(), &QObject::destroyed, this,
        [this, startupId, authorityEpoch, authenticationId]() {
            if (_startupAuthentication.value(QStringLiteral("startupId")).toString() != startupId
                || _startupAuthentication.value(QStringLiteral("authorityEpoch")).toString() != authorityEpoch
                || _startupAuthentication.value(QStringLiteral("authenticationId")).toString() != authenticationId) return;
            clearStartupAuthentication();
            emit changed(); // State committed; no member access afterwards.
        });
    if (!self || !authority || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || _startupAuthentication != receipt || _startupAuthenticationOwner != authority) {
        if (self && _startupAuthentication == receipt && _startupAuthenticationOwner == authority) clearStartupAuthentication();
        return startAuthenticationDenied(QStringLiteral("startup-authentication-episode-or-owner-changed-after-move"));
    }
    // No synchronous signal: both native owners must first commit the handoff.
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), receipt},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false},
        {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::startupAuthenticationCurrent(const QVariantMap& suppliedReceipt, QObject* authorityObject,
    const QString& suppliedStartupId, const QString& suppliedAuthorityEpoch, const QString& suppliedSourceHash) const
{
    const QVariantMap receipt = suppliedReceipt;
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QPointer<FleetMissionStartupAuthority> authority(qobject_cast<FleetMissionStartupAuthority*>(authorityObject));
    if (!ownerThread() || _promptActive || !authority || authority != _startupAuthenticationOwner
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || authority->thread() != thread() || authority->authorityEpoch() != authorityEpoch
        || !startupAuthenticationShape(receipt) || canonicalStartupAuthenticationNumbers(receipt) != _startupAuthentication
        || receipt.value(QStringLiteral("startupId")).toString() != startupId
        || receipt.value(QStringLiteral("authorityEpoch")).toString() != authorityEpoch
        || receipt.value(QStringLiteral("sourceHash")).toString() != sourceHash) return false;
    const QVariantMap primary = _startupPrimaryIdentity, approver = _startupApproverIdentity;
    if (!identitySnapshotCurrent(primary) || !self || !authority
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())) return false;
    const QString roleNow = currentRole(receipt.value(QStringLiteral("approverActorId")).toString());
    if (!self || !authority || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !workstationCurrent() || !self || !authority
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())) return false;
    return FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        && !_promptActive && authority == _startupAuthenticationOwner && authority->thread() == thread()
        && authority->authorityEpoch() == authorityEpoch && canonicalStartupAuthenticationNumbers(receipt) == _startupAuthentication
        && primary == _startupPrimaryIdentity && approver == _startupApproverIdentity && primary == _identityReceipt
        && receipt.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && receipt.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && receipt.value(QStringLiteral("primarySessionId")) == primary.value(QStringLiteral("sessionId"))
        && receipt.value(QStringLiteral("primaryActorId")) == primary.value(QStringLiteral("actorId"))
        && receipt.value(QStringLiteral("primaryRole")) == primary.value(QStringLiteral("role"))
        && receipt.value(QStringLiteral("approverActorId")) == approver.value(QStringLiteral("actorId"))
        && receipt.value(QStringLiteral("approverActorId")) != receipt.value(QStringLiteral("primaryActorId"))
        && approverRole(roleNow) && roleNow == receipt.value(QStringLiteral("approverRole")).toString()
        && receipt.value(QStringLiteral("approverRole")) == approver.value(QStringLiteral("role"))
        && receipt.value(QStringLiteral("approverAuthenticationId")) == approver.value(QStringLiteral("sessionId"))
        && receipt.value(QStringLiteral("bindingHash")) == approver.value(QStringLiteral("bindingHash"))
        && approver.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && receipt.value(QStringLiteral("originalAuthenticationExpiresAtMonotonicMs")).toDouble()
            == qMin(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())
        && _clock.elapsed() >= receipt.value(QStringLiteral("transferredAtMonotonicMs")).toDouble()
        && _clock.elapsed() < receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::revokeStartupAuthentication(QObject* authorityObject, const QString& suppliedStartupId,
                                                     const QString& suppliedAuthorityEpoch)
{
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch;
    if (!ownerThread() || !authorityObject || authorityObject != _startupAuthenticationOwner.data()
        || startupId.isEmpty() || startupId != _startupAuthentication.value(QStringLiteral("startupId")).toString()
        || authorityEpoch != _startupAuthentication.value(QStringLiteral("authorityEpoch")).toString()) return false;
    clearStartupAuthentication();
    return true;
}

void FleetOperatorSession::clearStartupTransactionAuthentication()
{
    QObject::disconnect(_startupTransactionOwnerDestroyedConnection);
    _startupTransactionOwnerDestroyedConnection = {};
    _startupTransactionAuthentication.clear(); _startupTransactionPrimaryIdentity.clear();
    _startupTransactionApproverIdentity.clear(); _startupTransactionOriginalBasis.clear();
    _startupTransactionAuthenticationOwner.clear();
}

QVariantMap FleetOperatorSession::transferStartupAuthenticationToTransaction(const QVariantMap& suppliedAuthentication,
    QObject* authorityObject, QObject* transactionObject, const QString& suppliedStartupId,
    const QString& suppliedAuthorityEpoch, const QString& suppliedTransactionEpoch, const QString& suppliedSourceHash,
    int maximumRemainingMs)
{
    const QVariantMap authentication = suppliedAuthentication;
    const QString startupId = suppliedStartupId, authorityEpoch = suppliedAuthorityEpoch;
    const QString transactionEpoch = suppliedTransactionEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(this);
    const QPointer<FleetMissionStartupAuthority> authority(qobject_cast<FleetMissionStartupAuthority*>(authorityObject));
    const QPointer<FleetMissionStartupTransaction> transaction(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    if (!ownerThread() || _promptActive || !authority || !transaction || authority->thread() != thread()
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || transaction->thread() != thread() || authority->authorityEpoch() != authorityEpoch
        || transaction->transactionEpoch() != transactionEpoch || maximumRemainingMs < 1 || maximumRemainingMs > 30000
        || QUuid(transactionEpoch).isNull() || QUuid(transactionEpoch).toString(QUuid::WithoutBraces) != transactionEpoch)
        return startAuthenticationDenied(QStringLiteral("actual-startup-transaction-owner-required"));
    const qint64 budgetDeadline = _clock.elapsed() + maximumRemainingMs;
    if (!startupAuthenticationCurrent(authentication, authority.data(), startupId, authorityEpoch, sourceHash)
        || !self || !authority || !transaction
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data()))
        return startAuthenticationDenied(QStringLiteral("authority-authentication-not-current"));
    const QVariantMap primary = _startupPrimaryIdentity, approver = _startupApproverIdentity;
    if (!_startupTransactionAuthentication.isEmpty()) {
        const QVariantMap previous = _startupTransactionAuthentication;
        const QPointer<FleetMissionStartupTransaction> previousOwner(_startupTransactionAuthenticationOwner);
        const bool current = startupTransactionAuthenticationCurrent(previous, previousOwner.data(),
            previous.value(QStringLiteral("startupId")).toString(), previous.value(QStringLiteral("transactionEpoch")).toString(),
            previous.value(QStringLiteral("sourceHash")).toString());
        if (!self) return startAuthenticationDenied(QStringLiteral("session-destroyed"));
        if (!authority || !transaction || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
            || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data()))
            return startAuthenticationDenied(QStringLiteral("startup-transaction-episode-changed-during-validation"));
        if (_startupTransactionAuthentication != previous || _startupTransactionAuthenticationOwner != previousOwner)
            return startAuthenticationDenied(QStringLiteral("startup-transaction-owner-changed-during-validation"));
        if (current) return startAuthenticationDenied(QStringLiteral("startup-transaction-authentication-already-owned"));
        clearStartupTransactionAuthentication();
    }
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || !startupAuthenticationCurrent(authentication, authority.data(), startupId, authorityEpoch, sourceHash)
        || !self || !authority || !transaction || primary != _startupPrimaryIdentity || approver != _startupApproverIdentity
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || transaction->thread() != thread() || transaction->transactionEpoch() != transactionEpoch)
        return startAuthenticationDenied(QStringLiteral("startup-transaction-authentication-source-changed"));
    const qint64 transferred = _clock.elapsed();
    const qint64 expires = qMin(budgetDeadline, qint64(authentication.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()));
    if (expires <= transferred) return startAuthenticationDenied(QStringLiteral("original-startup-authentication-budget-expired"));
    QVariantMap receipt = canonicalStartupAuthenticationNumbers(authentication);
    const QString authenticationId = uuid();
    receipt[QStringLiteral("authorityAuthenticationId")] = authentication.value(QStringLiteral("authenticationId"));
    receipt[QStringLiteral("authenticationId")] = authenticationId;
    receipt[QStringLiteral("transactionEpoch")] = transactionEpoch;
    receipt[QStringLiteral("transferredAtMonotonicMs")] = double(transferred);
    receipt[QStringLiteral("expiresAtMonotonicMs")] = double(expires);
    receipt[QStringLiteral("durationMs")] = double(expires - transferred);
    receipt[QStringLiteral("purpose")] = QStringLiteral("startup-transaction-authentication-basis-not-permit");
    if (!startupTransactionAuthenticationShape(receipt)) return startAuthenticationDenied(QStringLiteral("startup-transaction-authentication-shape-invalid"));
    if (!FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data()))
        return startAuthenticationDenied(QStringLiteral("startup-transaction-episode-changed-before-move"));
    clearStartupAuthentication(); // Original Authority proof is retired once, NOT an identity/TTL reset.
    _startupTransactionAuthentication = receipt; _startupTransactionPrimaryIdentity = primary;
    _startupTransactionApproverIdentity = approver; _startupTransactionOriginalBasis = canonicalStartupAuthenticationNumbers(authentication);
    _startupTransactionAuthenticationOwner = transaction;
    _startupTransactionOwnerDestroyedConnection = connect(transaction.data(), &QObject::destroyed, this,
        [this, startupId, transactionEpoch, authenticationId]() {
            if (_startupTransactionAuthentication.value(QStringLiteral("startupId")).toString() != startupId
                || _startupTransactionAuthentication.value(QStringLiteral("transactionEpoch")).toString() != transactionEpoch
                || _startupTransactionAuthentication.value(QStringLiteral("authenticationId")).toString() != authenticationId) return;
            clearStartupTransactionAuthentication();
            emit changed();
        });
    if (!self || !authority || !transaction || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, authority.data())
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || _startupTransactionAuthentication != receipt || _startupTransactionAuthenticationOwner != transaction) {
        if (self && _startupTransactionAuthentication == receipt && _startupTransactionAuthenticationOwner == transaction)
            clearStartupTransactionAuthentication();
        return startAuthenticationDenied(QStringLiteral("startup-transaction-episode-or-owner-changed-after-move"));
    }
    return {{QStringLiteral("ok"), true}, {QStringLiteral("authenticationReceipt"), receipt},
        {QStringLiteral("startupPermitGranted"), false}, {QStringLiteral("executable"), false}, {QStringLiteral("flightCommandReleased"), false}};
}

bool FleetOperatorSession::startupTransactionAuthenticationCurrent(const QVariantMap& suppliedReceipt,
    QObject* transactionObject, const QString& suppliedStartupId, const QString& suppliedTransactionEpoch,
    const QString& suppliedSourceHash) const
{
    const QVariantMap receipt = suppliedReceipt;
    const QString startupId = suppliedStartupId, transactionEpoch = suppliedTransactionEpoch, sourceHash = suppliedSourceHash;
    const QPointer<FleetOperatorSession> self(const_cast<FleetOperatorSession*>(this));
    const QPointer<FleetMissionStartupTransaction> transaction(qobject_cast<FleetMissionStartupTransaction*>(transactionObject));
    if (!ownerThread() || _promptActive || !transaction || transaction != _startupTransactionAuthenticationOwner
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || transaction->thread() != thread() || transaction->transactionEpoch() != transactionEpoch
        || !startupTransactionAuthenticationShape(receipt) || canonicalStartupAuthenticationNumbers(receipt) != _startupTransactionAuthentication
        || receipt.value(QStringLiteral("startupId")).toString() != startupId
        || receipt.value(QStringLiteral("transactionEpoch")).toString() != transactionEpoch
        || receipt.value(QStringLiteral("sourceHash")).toString() != sourceHash) return false;
    const QVariantMap primary = _startupTransactionPrimaryIdentity, approver = _startupTransactionApproverIdentity;
    const QVariantMap original = _startupTransactionOriginalBasis;
    if (!identitySnapshotCurrent(primary) || !self || !transaction
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())) return false;
    const QString role = currentRole(receipt.value(QStringLiteral("approverActorId")).toString());
    if (!self || !transaction || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        || !workstationCurrent() || !self || !transaction
        || !FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())) return false;
    return FleetMissionDeploymentEpisode::outboundOwnersCompatible(this, transaction.data())
        && !_promptActive && transaction == _startupTransactionAuthenticationOwner && transaction->thread() == thread()
        && transaction->transactionEpoch() == transactionEpoch && canonicalStartupAuthenticationNumbers(receipt) == _startupTransactionAuthentication
        && primary == _startupTransactionPrimaryIdentity && approver == _startupTransactionApproverIdentity
        && original == _startupTransactionOriginalBasis && primary == _identityReceipt
        && receipt.value(QStringLiteral("authorityEpoch")) == original.value(QStringLiteral("authorityEpoch"))
        && receipt.value(QStringLiteral("authorityAuthenticationId")) == original.value(QStringLiteral("authenticationId"))
        && receipt.value(QStringLiteral("approvalId")) == original.value(QStringLiteral("approvalId"))
        && receipt.value(QStringLiteral("sourceHash")) == original.value(QStringLiteral("sourceHash"))
        && receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble() <= original.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
        && receipt.value(QStringLiteral("sessionEpoch")).toString() == _sessionEpoch
        && receipt.value(QStringLiteral("sessionRevision")).toString() == QString::number(_sessionRevision)
        && receipt.value(QStringLiteral("primarySessionId")) == primary.value(QStringLiteral("sessionId"))
        && receipt.value(QStringLiteral("primaryActorId")) == primary.value(QStringLiteral("actorId"))
        && receipt.value(QStringLiteral("primaryRole")) == primary.value(QStringLiteral("role"))
        && receipt.value(QStringLiteral("approverActorId")) == approver.value(QStringLiteral("actorId"))
        && receipt.value(QStringLiteral("approverActorId")) != receipt.value(QStringLiteral("primaryActorId"))
        && approverRole(role) && role == receipt.value(QStringLiteral("approverRole")).toString()
        && receipt.value(QStringLiteral("approverRole")) == approver.value(QStringLiteral("role"))
        && receipt.value(QStringLiteral("approverAuthenticationId")) == approver.value(QStringLiteral("sessionId"))
        && receipt.value(QStringLiteral("bindingHash")) == approver.value(QStringLiteral("bindingHash"))
        && approver.value(QStringLiteral("action")).toString() == QStringLiteral("mission.start")
        && receipt.value(QStringLiteral("originalAuthenticationExpiresAtMonotonicMs")).toDouble()
            == qMin(primary.value(QStringLiteral("expiresAtMonotonicMs")).toDouble(), approver.value(QStringLiteral("expiresAtMonotonicMs")).toDouble())
        && _clock.elapsed() >= receipt.value(QStringLiteral("transferredAtMonotonicMs")).toDouble()
        && _clock.elapsed() < receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
}

bool FleetOperatorSession::revokeStartupTransactionAuthentication(QObject* transactionObject,
    const QString& suppliedStartupId, const QString& suppliedTransactionEpoch)
{
    const QString startupId = suppliedStartupId, transactionEpoch = suppliedTransactionEpoch;
    if (!ownerThread() || !transactionObject || transactionObject != _startupTransactionAuthenticationOwner.data()
        || startupId.isEmpty() || startupId != _startupTransactionAuthentication.value(QStringLiteral("startupId")).toString()
        || transactionEpoch != _startupTransactionAuthentication.value(QStringLiteral("transactionEpoch")).toString()) return false;
    clearStartupTransactionAuthentication();
    return true;
}

void FleetOperatorSession::expireReceipts()
{
    if (!ownerThread()) return;
    QPointer<FleetOperatorSession> self(this);
    bool didChange = false;
    if (!_identityReceipt.isEmpty()
        && _clock.elapsed() >= _identityReceipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) {
        advanceSession();
        _state = QStringLiteral("expired");
        _lastError = QStringLiteral("operator-session-expired");
        didChange = true;
    } else if (!_approverReceipt.isEmpty()
        && _clock.elapsed() >= _approverReceipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()) {
        _approverReceipt.clear();
        _approvedApplicantIdentity.clear();
        didChange = true;
    }
    // OS role edits have no Qt signal. Poll only outside the nested credential
    // prompt, revoke rather than renew receipts, and still re-read natively at
    // every action gate. Modal-return validation covers edits during a prompt.
    if (!_promptActive && !_identityReceipt.isEmpty() && !identityCurrent(_identityReceipt, true)) {
        if (!self) return;
        advanceSession();
        _state = QStringLiteral("revoked");
        _lastError = QStringLiteral("identity-source-role-not-current");
        didChange = true;
    } else if (!_promptActive && !_approverReceipt.isEmpty()
               && !approverSnapshotCurrent(_approverReceipt)) {
        if (!self) return;
        _approverReceipt.clear();
        _approvedApplicantIdentity.clear();
        didChange = true;
    }
    if (!self) return;
    if (!_telemetryConfigurationBindingHash.isEmpty()
        && (!_telemetryConfigurationOwner || _clock.elapsed() >= _telemetryConfigurationDeadlineMs)) {
        clearTelemetryConfigurationAuthentication(); didChange = true;
    }
    if (!_cameraStreamControlBindingHash.isEmpty()
        && (!_cameraStreamControlOwner || _clock.elapsed() >= _cameraStreamControlDeadlineMs)) {
        clearCameraStreamControlAuthentication(); didChange = true;
    }
    if (!_uploadIdentityLease.isEmpty() && !uploadIdentityLeaseCurrent(_uploadIdentityLease)) {
        if (!self) return;
        clearUploadIdentityLease();
        didChange = true;
    }
    if (!self) return;
    // Unlike the historical basis of an in-flight upload lease, a consumed
    // start-intent authentication never outlives the original approver window.
    // Busy alone denies its gate but is not an automatic TTL extension/revoke.
    if (!_startApprovalAuthentication.isEmpty()
        && (_clock.elapsed() >= _startApprovalAuthentication.value(QStringLiteral("expiresAtMonotonicMs")).toDouble()
            || (!_promptActive && !startApprovalAuthenticationCurrent(_startApprovalAuthentication)))) {
        if (!self) return;
        clearStartApprovalAuthentication();
        didChange = true;
    }
    if (!self) return;
    if (!_groundStartApprovalAuthentication.isEmpty()) {
        const QVariantMap receipt = _groundStartApprovalAuthentication;
        const QPointer<FleetMissionGroundStartApproval> owner(_groundStartApprovalOwner);
        const bool expired = _clock.elapsed() >= receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
        const bool current = !expired && owner && (_promptActive || groundStartApprovalAuthenticationCurrent(receipt, owner.data(),
            receipt.value(QStringLiteral("approvalId")).toString(), receipt.value(QStringLiteral("approvalEpoch")).toString(),
            receipt.value(QStringLiteral("bindingHash")).toString(), receipt.value(QStringLiteral("reviewId")).toString(),
            receipt.value(QStringLiteral("reviewEpoch")).toString()));
        if (!self) return;
        if (!current && _groundStartApprovalAuthentication == receipt && _groundStartApprovalOwner == owner) {
            clearGroundStartApprovalAuthentication(); didChange = true;
        }
    }
    if (!self) return;
    if (!_groundStartupAuthentication.isEmpty()) {
        const QVariantMap receipt = _groundStartupAuthentication;
        const QPointer<FleetMissionGroundStartupAuthority> authority(_groundStartupOwner);
        const bool expired = _clock.elapsed() >= receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
        const bool current = !expired && authority && (_promptActive || groundStartupAuthenticationCurrent(receipt, authority.data(),
            receipt.value(QStringLiteral("startupId")).toString(), receipt.value(QStringLiteral("authorityEpoch")).toString(),
            receipt.value(QStringLiteral("sourceHash")).toString()));
        if (!self) return;
        if (!current && _groundStartupAuthentication == receipt && _groundStartupOwner == authority) {
            clearGroundStartupAuthentication(); didChange = true;
        }
    }
    if (!self) return;
    if (!_groundStartupTransactionAuthentication.isEmpty()) {
        const QVariantMap receipt = _groundStartupTransactionAuthentication;
        const QPointer<FleetMissionStartupTransaction> transaction(_groundStartupTransactionOwner);
        const bool expired = _clock.elapsed() >= receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
        const bool current = !expired && transaction && (_promptActive || groundStartupTransactionAuthenticationCurrent(receipt, transaction.data(),
            receipt.value(QStringLiteral("startupId")).toString(), receipt.value(QStringLiteral("transactionEpoch")).toString(),
            receipt.value(QStringLiteral("sourceHash")).toString()));
        if (!self) return;
        if (!current && _groundStartupTransactionAuthentication == receipt && _groundStartupTransactionOwner == transaction) {
            clearGroundStartupTransactionAuthentication(); didChange = true;
        }
    }
    if (!self) return;
    if (!_startupAuthentication.isEmpty()) {
        const QVariantMap receipt = _startupAuthentication;
        const QPointer<FleetMissionStartupAuthority> authority(_startupAuthenticationOwner);
        const bool expired = _clock.elapsed() >= receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
        const bool current = !expired && authority && (_promptActive || startupAuthenticationCurrent(receipt, authority.data(),
            receipt.value(QStringLiteral("startupId")).toString(), receipt.value(QStringLiteral("authorityEpoch")).toString(),
            receipt.value(QStringLiteral("sourceHash")).toString()));
        if (!self) return;
        if (!current && _startupAuthentication == receipt && _startupAuthenticationOwner == authority) {
            clearStartupAuthentication(); didChange = true;
        }
    }
    if (!self) return;
    if (!_startupTransactionAuthentication.isEmpty()) {
        const QVariantMap receipt = _startupTransactionAuthentication;
        const QPointer<FleetMissionStartupTransaction> transaction(_startupTransactionAuthenticationOwner);
        const bool expired = _clock.elapsed() >= receipt.value(QStringLiteral("expiresAtMonotonicMs")).toDouble();
        const bool current = !expired && transaction && (_promptActive || startupTransactionAuthenticationCurrent(receipt, transaction.data(),
            receipt.value(QStringLiteral("startupId")).toString(), receipt.value(QStringLiteral("transactionEpoch")).toString(),
            receipt.value(QStringLiteral("sourceHash")).toString()));
        if (!self) return;
        if (!current && _startupTransactionAuthentication == receipt && _startupTransactionAuthenticationOwner == transaction) {
            clearStartupTransactionAuthentication(); didChange = true;
        }
    }
    if (didChange) emit changed();
}
