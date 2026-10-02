#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
class FleetMissionSimulator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList steps READ steps WRITE setSteps NOTIFY stepsChanged)
    Q_PROPERTY(QVariantList faults READ faults NOTIFY faultsChanged)
public:
    explicit FleetMissionSimulator(QObject* parent = nullptr);
    QVariantList steps() const { return _steps; } QVariantList faults() const { return _faults; }
    void setSteps(const QVariantList& steps);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void injectFault(const QString& kind, const QVariantMap& details = QVariantMap());
    Q_INVOKABLE QVariantList preview() const;
signals: void stepsChanged(); void faultsChanged(); void previewReady(const QVariantList& result);
private: QVariantList _steps; QVariantList _faults;
};
