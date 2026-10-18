#pragma once

#include <QObject>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Catalog of reusable industry task templates.  Templates describe intent,
// constraints and defaults; they do not create or dispatch a flight mission.
class FleetTaskTemplateRegistry : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int templateCount READ templateCount NOTIFY templatesChanged)
    Q_PROPERTY(QVariantList templates READ templates NOTIFY templatesChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion CONSTANT)

public:
    explicit FleetTaskTemplateRegistry(QObject* parent = nullptr);

    int templateCount() const { return _templates.size(); }
    int schemaVersion() const { return kSchemaVersion; }
    QVariantList templates() const;

    /// C++ catalog-management API. Intentionally not Q_INVOKABLE: QML may
    /// consume and validate templates, but cannot forge, remove, enable, or
    /// replace the task catalog behind the review boundary.
    bool registerTemplate(const QString& templateId,
                          const QString& displayName,
                          const QString& category,
                          const QString& taskType,
                          const QStringList& requiredCapabilities = QStringList(),
                          int recommendedVehicleCount = 1,
                          int maxDurationSeconds = 0,
                          const QVariantMap& defaults = QVariantMap(),
                          const QString& description = QString());
    // Extended registration is for a trusted C++ catalog synchronizer and
    // carries version and trust metadata without exposing a QML write path.
    bool registerVersionedTemplate(const QString& templateId,
                                   const QString& displayName,
                                   const QString& category,
                                   const QString& taskType,
                                   const QStringList& requiredCapabilities,
                                   int recommendedVehicleCount,
                                   int maxDurationSeconds,
                                   const QVariantMap& defaults,
                                   const QString& description,
                                   const QString& version = QStringLiteral("1.0.0"),
                                   const QVariantMap& constraints = QVariantMap(),
                                   const QString& signature = QString());
    bool removeTemplate(const QString& templateId);
    bool setTemplateEnabled(const QString& templateId, bool enabled);
    Q_INVOKABLE QVariantMap templateDefinition(const QString& templateId) const;
    Q_INVOKABLE QVariantList findByCategory(const QString& category) const;
    Q_INVOKABLE QVariantList findByCapability(const QString& capability) const;
    Q_INVOKABLE bool hasTemplate(const QString& templateId) const;

    // Validates a task request against the selected template and a read-only
    // fleet snapshot.  This produces review data only; it never selects a
    // Vehicle, changes a task, or releases a flight command.
    Q_INVOKABLE QVariantMap validateTask(const QString& templateId,
                                         const QVariantMap& task,
                                         const QVariantList& fleet) const;

    // Portable, versioned registry snapshots.  Signature fields are a
    // deliberately inert placeholder until a trusted signing service is
    // integrated by the product layer.
    Q_INVOKABLE QString exportJson() const;
    bool importJson(const QString& json);
    bool saveToFile(const QString& filePath) const;
    bool loadFromFile(const QString& filePath);

signals:
    void templatesChanged();
    void templateRegistered(const QString& templateId, const QVariantMap& definition);
    void templateRemoved(const QString& templateId);

private:
    struct Definition {
        QString id;
        QString displayName;
        QString category;
        QString taskType;
        QStringList requiredCapabilities;
        int recommendedVehicleCount = 1;
        int maxDurationSeconds = 0;
        QVariantMap defaults;
        QVariantMap constraints;
        QString version;
        QString signature;
        QString description;
        bool enabled = true;
    };

    static const int kSchemaVersion = 1;
    static const int kMaximumTemplates = 500;

    static QString normalize(const QString& value);
    static QStringList normalizedList(const QVariant& value);
    static bool validVersion(const QString& value);
    static QVariantMap signaturePlaceholder(const QString& signature);
    QVariantMap toVariantMap(const Definition& definition) const;
    static bool definitionFromVariant(const QVariantMap& value, Definition* definition);

    QHash<QString, Definition> _templates;
};
