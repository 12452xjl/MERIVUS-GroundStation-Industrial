/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QQmlEngine>
#include <cmath>

#include "CustomActionManager.h"
#include "CustomAction.h"
#include "JsonHelper.h"
#include "QGCApplication.h"
#include "SettingsManager.h"

CustomActionManager::CustomActionManager(void) {
    auto flyViewSettings = qgcApp()->toolbox()->settingsManager()->flyViewSettings();
    Fact* customActionsFact = flyViewSettings->customActionDefinitions();

    connect(customActionsFact, &Fact::valueChanged, this, &CustomActionManager::_loadFromJson);

    // On construction, we only load the Custom Actions if we have a path
    // defined, to prevent spurious warnings.
    if (!customActionsFact->rawValue().toString().isEmpty()) {
        _loadFromJson(customActionsFact->rawValue());
    }
}

void CustomActionManager::_loadFromJson(QVariant fact) {
    QString path = fact.toString();

    const char* kQgcFileType = "CustomActions";
    const char* kActionListKey = "actions";

    _actions.clearAndDeleteContents();

    QString errorString;
    int version;
    QJsonObject jsonObject = JsonHelper::openInternalQGCJsonFile(path, kQgcFileType, 1, 1, version, errorString);
    if (!errorString.isEmpty()) {
        qCWarning(GuidedActionsControllerLog) << "Custom Actions Internal Error: " << errorString;
        emit actionsChanged();
        return;
    }

    QList<JsonHelper::KeyValidateInfo> keyInfoList = {
        { kActionListKey, QJsonValue::Array, /* required= */ true },
    };
    if (!JsonHelper::validateKeys(jsonObject, keyInfoList, errorString)) {
        qCWarning(GuidedActionsControllerLog) << "Custom Actions JSON document incorrect format:" << errorString;
        emit actionsChanged();
        return;
    }

    QJsonArray actionList = jsonObject[kActionListKey].toArray();
    for (auto actionJson: actionList) {
        if (!actionJson.isObject()) {
            qCWarning(GuidedActionsControllerLog) << "Custom Actions JsonValue not an object: " << actionJson;
            continue;
        }

        auto actionObj = actionJson.toObject();

        QList<JsonHelper::KeyValidateInfo> actionKeyInfoList = {
            { "label",  QJsonValue::String, /* required= */ true },
            { "mavCmd", QJsonValue::Double, /* required= */ true },

            { "compId", QJsonValue::Double, /* required= */ false },
            { "param1", QJsonValue::Double, /* required= */ false },
            { "param2", QJsonValue::Double, /* required= */ false },
            { "param3", QJsonValue::Double, /* required= */ false },
            { "param4", QJsonValue::Double, /* required= */ false },
            { "param5", QJsonValue::Double, /* required= */ false },
            { "param6", QJsonValue::Double, /* required= */ false },
            { "param7", QJsonValue::Double, /* required= */ false },
            { "safetyClass", QJsonValue::String, /* required= */ false },
        };
        if (!JsonHelper::validateKeys(actionObj, actionKeyInfoList, errorString)) {
            qCWarning(GuidedActionsControllerLog) << "Custom Actions JSON document incorrect format:" << errorString;
            continue;
        }

        const int mavCmdValue = actionObj["mavCmd"].toInt(-1);
        const int compIdValue = actionObj["compId"].toInt(MAV_COMP_ID_AUTOPILOT1);
        if (mavCmdValue <= 0 || mavCmdValue > 65535 || compIdValue <= 0 || compIdValue > 255) {
            qCWarning(GuidedActionsControllerLog) << "Custom action rejected invalid command/component" << mavCmdValue << compIdValue;
            continue;
        }

        const QJsonValue paramValues[] = {
            actionObj["param1"], actionObj["param2"], actionObj["param3"],
            actionObj["param4"], actionObj["param5"], actionObj["param6"],
            actionObj["param7"]
        };
        bool finiteParams = true;
        for (const QJsonValue& paramValue : paramValues) {
            if (!paramValue.isUndefined() && !std::isfinite(paramValue.toDouble())) {
                finiteParams = false;
                break;
            }
        }
        if (!finiteParams) {
            qCWarning(GuidedActionsControllerLog) << "Custom action rejected non-finite parameter";
            continue;
        }

        auto label = actionObj["label"].toString();
        auto mavCmd = (MAV_CMD)mavCmdValue;
        auto compId = (MAV_COMPONENT)compIdValue;
        auto param1 = actionObj["param1"].toDouble(0.0);
        auto param2 = actionObj["param2"].toDouble(0.0);
        auto param3 = actionObj["param3"].toDouble(0.0);
        auto param4 = actionObj["param4"].toDouble(0.0);
        auto param5 = actionObj["param5"].toDouble(0.0);
        auto param6 = actionObj["param6"].toDouble(0.0);
        auto param7 = actionObj["param7"].toDouble(0.0);
        auto safetyClass = actionObj["safetyClass"].toString();

        CustomAction* action = new CustomAction(label, mavCmd, compId, param1, param2, param3, param4, param5, param6, param7, safetyClass);
        QQmlEngine::setObjectOwnership(action, QQmlEngine::CppOwnership);
        _actions.append(action);
    }

    emit actionsChanged();
}
