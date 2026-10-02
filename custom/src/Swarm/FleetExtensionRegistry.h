#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

// Metadata registry for optional Fleet OS extensions.  It advertises which
// integrations are installed and enabled, but deliberately does not load
// arbitrary binaries or execute a plugin callback.  A trusted adapter may
// consume this registry and apply its own signing, sandboxing and permission
// checks before activating an extension.
class FleetExtensionRegistry : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int extensionCount READ extensionCount NOTIFY extensionsChanged)
    Q_PROPERTY(QVariantList extensions READ extensions NOTIFY extensionsChanged)

public:
    explicit FleetExtensionRegistry(QObject* parent = nullptr);

    int extensionCount() const { return _extensions.size(); }
    QVariantList extensions() const;

    Q_INVOKABLE bool registerExtension(const QString& extensionId,
                                       const QString& displayName,
                                       const QString& version,
                                       const QString& extensionType,
                                       const QStringList& capabilities = QStringList(),
                                       const QString& entryPoint = QString(),
                                       const QVariantMap& metadata = QVariantMap());
    Q_INVOKABLE bool removeExtension(const QString& extensionId);
    Q_INVOKABLE bool setExtensionEnabled(const QString& extensionId, bool enabled);
    Q_INVOKABLE QVariantMap extension(const QString& extensionId) const;
    Q_INVOKABLE QVariantList findByType(const QString& extensionType) const;
    Q_INVOKABLE QVariantList findByCapability(const QString& capability) const;
    Q_INVOKABLE bool hasExtension(const QString& extensionId) const;

signals:
    void extensionsChanged();
    void extensionRegistered(const QString& extensionId, const QVariantMap& summary);
    void extensionRemoved(const QString& extensionId);

private:
    struct Extension {
        QString id;
        QString displayName;
        QString version;
        QString type;
        QStringList capabilities;
        QString entryPoint;
        QVariantMap metadata;
        bool enabled = false;
    };

    static QString normalize(const QString& value);
    QVariantMap toVariantMap(const Extension& extension) const;

    QHash<QString, Extension> _extensions;
};
