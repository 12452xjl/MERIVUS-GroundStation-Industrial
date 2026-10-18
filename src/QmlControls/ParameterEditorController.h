/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QObject>
#include <QList>
#include <QPointer>
#include <memory>

#include "AutoPilotPlugin.h"
#include "UASInterface.h"
#include "FactPanelController.h"
#include "QmlObjectListModel.h"
#include "ParameterManager.h"

class ParameterEditorGroup : public QObject
{
    Q_OBJECT

public:
    ParameterEditorGroup(QObject* parent) : QObject(parent) { }

    Q_PROPERTY(QString              name    MEMBER name     CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  facts   READ getFacts   CONSTANT)

    QmlObjectListModel*  getFacts(void) { return &facts; }

    int                 componentId;
    QString             name;
    QmlObjectListModel  facts;
};

class ParameterEditorCategory : public QObject
{
    Q_OBJECT

public:
    ParameterEditorCategory(QObject* parent) : QObject(parent) { }

    Q_PROPERTY(QString              name    MEMBER name     CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  groups  READ getGroups  CONSTANT)

    QmlObjectListModel*  getGroups(void) { return &groups; }

    QString             name;
    QmlObjectListModel  groups;
    QMap<QString, ParameterEditorGroup*> mapGroupName2Group;
};

class ParameterEditorDiff : public QObject
{
    Q_OBJECT

public:
    ParameterEditorDiff(QObject* parent) : QObject(parent) { }

    Q_PROPERTY(int      componentId         MEMBER componentId      CONSTANT)
    Q_PROPERTY(QString  name                MEMBER name             CONSTANT)
    Q_PROPERTY(QString  fileValue           MEMBER fileValue        CONSTANT)
    Q_PROPERTY(QString  vehicleValue        MEMBER vehicleValue     CONSTANT)
    Q_PROPERTY(bool     noVehicleValue      MEMBER noVehicleValue   CONSTANT)
    Q_PROPERTY(QString  units               MEMBER units            CONSTANT)
    Q_PROPERTY(bool     load                MEMBER load             NOTIFY loadChanged)

    int                         componentId;
    QString                     name;
    FactMetaData::ValueType_t   valueType;
    QString                     fileValue;
    QVariant                    fileValueVar;
    QString                     vehicleValue;
    bool                        noVehicleValue  = false;
    QString                     units;
    bool                        load            = true;

signals:
    void loadChanged(bool load);
};

class ParameterEditorController : public FactPanelController
{
    Q_OBJECT

public:
    ParameterEditorController(void);
    ~ParameterEditorController();

    Q_PROPERTY(QString              searchText          MEMBER _searchText                                          NOTIFY searchTextChanged)
    Q_PROPERTY(QmlObjectListModel*  categories          READ categories                                             CONSTANT)
    Q_PROPERTY(QObject*             currentCategory     READ currentCategory        WRITE setCurrentCategory        NOTIFY currentCategoryChanged)
    Q_PROPERTY(QObject*             currentGroup        READ currentGroup           WRITE setCurrentGroup           NOTIFY currentGroupChanged)
    Q_PROPERTY(QmlObjectListModel*  parameters          MEMBER _parameters                                          NOTIFY parametersChanged)
    Q_PROPERTY(bool                 showModifiedOnly    MEMBER _showModifiedOnly                                    NOTIFY showModifiedOnlyChanged)

    // These property are related to the diff associated with a load from file
    Q_PROPERTY(bool                 diffOtherVehicle        MEMBER _diffOtherVehicle        NOTIFY diffOtherVehicleChanged)
    Q_PROPERTY(bool                 diffMultipleComponents  MEMBER _diffMultipleComponents  NOTIFY diffMultipleComponentsChanged)
    Q_PROPERTY(QmlObjectListModel*  diffList                READ diffList                   CONSTANT)
    Q_PROPERTY(QString              diffError               READ diffError                  NOTIFY diffErrorChanged)
    Q_PROPERTY(QString              diffToken               READ diffToken                  NOTIFY diffTokenChanged)
    Q_PROPERTY(QVariantMap          parameterImportState    READ parameterImportState       NOTIFY parameterImportChanged)

    Q_INVOKABLE QStringList searchParameters(const QString& searchText, bool searchInName=true, bool searchInDescriptions=true);

    Q_INVOKABLE void saveToFile                     (const QString& filename);
    Q_INVOKABLE bool buildDiffFromFile              (const QString& filename);
    Q_INVOKABLE void clearDiff                      (const QString& expectedToken);
    Q_INVOKABLE bool sendDiff                       (const QString& expectedToken);
    Q_INVOKABLE bool cancelParameterImport          (const QString& runId);
    Q_INVOKABLE void refresh                        (void);
    Q_INVOKABLE bool resetAllToDefaults             (void);
    Q_INVOKABLE bool resetAllToVehicleConfiguration (void);
    Q_INVOKABLE bool writeAllowed                   (void) const;
    Q_INVOKABLE QString writeBlockReason            (void) const;

    QObject*            currentCategory     (void) { return _currentCategory; }
    QObject*            currentGroup        (void) { return _currentGroup; }
    QmlObjectListModel* categories          (void) { return &_categories; }
    QmlObjectListModel* diffList            (void) { return &_diffList; }
    QString             diffError           (void) const { return _diffError; }
    QString             diffToken           (void) const { return _diffToken; }
    QVariantMap         parameterImportState (void) const;
    void                setCurrentCategory  (QObject* currentCategory);
    void                setCurrentGroup     (QObject* currentGroup);

signals:
    void searchTextChanged              (QString searchText);
    void currentCategoryChanged         (void);
    void currentGroupChanged            (void);
    void showModifiedOnlyChanged        (void);
    void diffOtherVehicleChanged        (bool diffOtherVehicle);
    void diffMultipleComponentsChanged  (bool diffMultipleComponents);
    void parametersChanged              (void);
    void diffErrorChanged               (void);
    void diffTokenChanged               (void);
    void parameterImportChanged         (void);

private slots:
    void _currentCategoryChanged(void);
    void _currentGroupChanged   (void);
    void _searchTextChanged     (void);
    void _buildLists            (void);
    void _buildListsForComponent(int compId);
    void _factAdded             (int compId, Fact* fact);

private:
    bool _shouldShow(Fact *fact) const;
    QString _writeBlockReason(void) const;
    struct FilePreview;
    std::shared_ptr<FilePreview> _captureFilePreview();
    bool _filePreviewCurrent(const std::shared_ptr<FilePreview>& preview) const;
    bool _clearDiffPrivate();
    bool _diffFailure(const QString& error);

private:
    QPointer<ParameterManager>  _parameterMgr;
    QString                     _searchText;
    ParameterEditorCategory*    _currentCategory        = nullptr;
    ParameterEditorGroup*       _currentGroup           = nullptr;
    bool                        _showModifiedOnly       = false;
    bool                        _diffOtherVehicle       = false;
    bool                        _diffMultipleComponents = false;
    int                         _diffVehicleId          = -1;
    QString                     _diffError;
    QString                     _diffToken;
    bool                        _diffBusy               = false;
    std::shared_ptr<FilePreview> _filePreview;

    QmlObjectListModel          _categories;
    QmlObjectListModel          _diffList;
    QmlObjectListModel          _searchParameters;
    QmlObjectListModel*         _parameters             = nullptr;
    QMap<QString, ParameterEditorCategory*> _mapCategoryName2Category;
};
