#pragma once

#include <QObject>
#include <QHash>
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

public:
    explicit FleetTaskTemplateRegistry(QObject* parent = nullptr);

    int templateCount() const { return _templates.size(); }
    QVariantList templates() const;

    Q_INVOKABLE bool registerTemplate(const QString& templateId,
                                      const QString& displayName,
                                      const QString& category,
                                      const QString& taskType,
                                      const QStringList& requiredCapabilities = QStringList(),
                                      int recommendedVehicleCount = 1,
                                      int maxDurationSeconds = 0,
                                      const QVariantMap& defaults = QVariantMap(),
                                      const QString& description = QString());
    Q_INVOKABLE bool removeTemplate(const QString& templateId);
    Q_INVOKABLE bool setTemplateEnabled(const QString& templateId, bool enabled);
    Q_INVOKABLE QVariantMap templateDefinition(const QString& templateId) const;
    Q_INVOKABLE QVariantList findByCategory(const QString& category) const;
    Q_INVOKABLE QVariantList findByCapability(const QString& capability) const;
    Q_INVOKABLE bool hasTemplate(const QString& templateId) const;

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
        QString description;
        bool enabled = true;
    };

    static QString normalize(const QString& value);
    QVariantMap toVariantMap(const Definition& definition) const;

    QHash<QString, Definition> _templates;
};
