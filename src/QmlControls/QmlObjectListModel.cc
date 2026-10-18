/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "QmlObjectListModel.h"

#include <QDebug>
#include <QPointer>
#include <QQmlEngine>

const int QmlObjectListModel::ObjectRole = Qt::UserRole;
const int QmlObjectListModel::TextRole = Qt::UserRole + 1;

QmlObjectListModel::QmlObjectListModel(QObject* parent)
    : QAbstractListModel        (parent)
    , _dirty                    (false)
    , _skipDirtyFirstItem       (false)
    , _externalBeginResetModel  (false)
{

}

QmlObjectListModel::~QmlObjectListModel()
{
    
}

QObject* QmlObjectListModel::get(int index)
{
    if (index < 0 || index >= _objectList.count()) {
        return nullptr;
    }
    return _objectList[index];
}

int QmlObjectListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    
    return _objectList.count();
}

QVariant QmlObjectListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    
    if (index.row() < 0 || index.row() >= _objectList.count()) {
        return QVariant();
    }
    
    if (role == ObjectRole) {
        return QVariant::fromValue(_objectList[index.row()]);
    } else if (role == TextRole) {
        QObject* const object = _objectList.at(index.row());
        return object ? QVariant::fromValue(object->objectName()) : QVariant();
    } else {
        return QVariant();
    }
}

QHash<int, QByteArray> QmlObjectListModel::roleNames(void) const
{
    QHash<int, QByteArray> hash;
    
    hash[ObjectRole] = "object";
    hash[TextRole] = "text";
    
    return hash;
}

bool QmlObjectListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() && role == ObjectRole) {
        _objectList.replace(index.row(), value.value<QObject*>());
        emit dataChanged(index, index);
        return true;
    }
    
    return false;
}

bool QmlObjectListModel::insertRows(int position, int rows, const QModelIndex& parent)
{
    Q_UNUSED(parent);

    if (position < 0 || rows < 1 || position > _objectList.count() || rows > _objectList.count() - position) {
        qWarning() << "Invalid position position:count" << position << _objectList.count();
        return false;
    }

    const QPointer<QmlObjectListModel> self(this);
    // These notifications are synchronous lifetime boundaries. Callers own
    // any operation/mutation fence; this does not change raw list ownership.
    beginInsertRows(QModelIndex(), position, position + rows - 1);
    if (!self) return false;
    endInsertRows();
    if (!self) return false;
    emit countChanged(count());

    return bool(self);
}

bool QmlObjectListModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    Q_UNUSED(parent);
    
    if (position < 0 || rows < 1 || position >= _objectList.count()) {
        qWarning() << "Invalid position position:count" << position << _objectList.count();
        return false;
    } else if (rows > _objectList.count() - position) {
        qWarning() << "Invalid rows position:rows:count" << position << rows << _objectList.count();
        return false;
    }

    const QPointer<QmlObjectListModel> self(this);
    beginRemoveRows(QModelIndex(), position, position + rows - 1);
    if (!self) return false;
    for (int row=0; row<rows; row++) {
        _objectList.removeAt(position);
    }
    endRemoveRows();
    if (!self) return false;
    emit countChanged(count());

    return bool(self);
}

void QmlObjectListModel::move(int from, int to)
{
    if(0 <= from && from < count() && 0 <= to && to < count() && from != to) {
        // Workaround to allow move item to the bottom. Done according to
        // beginMoveRows() documentation and implementation specificity:
        // https://doc.qt.io/qt-5/qabstractitemmodel.html#beginMoveRows
        // (see 3rd picture explanation there)
        if(from == to - 1) {
            to = from++;
        }
        const QPointer<QmlObjectListModel> self(this);
        const bool begun = beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
        if (!self || !begun) return;
        _objectList.move(from, to);
        endMoveRows();
        if (!self) return;
    }
}

QObject* QmlObjectListModel::operator[](int index)
{
    if (index < 0 || index >= _objectList.count()) {
        return nullptr;
    }
    return _objectList[index];
}

const QObject* QmlObjectListModel::operator[](int index) const
{
    if (index < 0 || index >= _objectList.count()) {
        return nullptr;
    }
    return _objectList[index];
}

void QmlObjectListModel::clear()
{
    const QPointer<QmlObjectListModel> self(this);
    const bool externalReset = _externalBeginResetModel;
    if (!externalReset) {
        beginResetModel();
        if (!self) return;
    }
    _objectList.clear();
    if (!externalReset) {
        endResetModel();
        if (!self) return;
        emit countChanged(count());
    }
}

QObject* QmlObjectListModel::removeAt(int i)
{
    if (i < 0 || i >= _objectList.count()) return nullptr;
    const QPointer<QmlObjectListModel> self(this);
    QObject* const original = _objectList.at(i);
    const QPointer<QObject> removedObject(original);
    if(removedObject) {
        // Look for a dirtyChanged signal on the object
        const bool hasDirtySignal = removedObject->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)")) != -1;
        if (!self) return nullptr;
        if (removedObject && hasDirtySignal) {
            if (!_skipDirtyFirstItem || i != 0) {
                QObject::disconnect(removedObject.data(), SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool)));
                if (!self) return nullptr;
            }
        }
    }
    if (i >= _objectList.count() || _objectList.at(i) != original || !removeRows(i, 1) || !self) return nullptr;
    setDirty(true);
    return self ? removedObject.data() : nullptr;
}

void QmlObjectListModel::insert(int i, QObject* object)
{
    if (i < 0 || i > _objectList.count()) {
        qWarning() << "Invalid index index:count" << i << _objectList.count();
        return;
    }
    const QPointer<QmlObjectListModel> self(this);
    const QPointer<QObject> actualObject(object);
    QMetaObject::Connection connection;
    if(actualObject) {
        QQmlEngine::setObjectOwnership(actualObject.data(), QQmlEngine::CppOwnership);
        if (!self || !actualObject) return;
        // Look for a dirtyChanged signal on the object
        const bool hasDirtySignal = actualObject->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)")) != -1;
        if (!self || !actualObject) return;
        if (hasDirtySignal) {
            if (!_skipDirtyFirstItem || i != 0) {
                connection = QObject::connect(actualObject.data(), SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool)));
                if (!self || !actualObject) { QObject::disconnect(connection); return; }
            }
        }
    }
    if (i > _objectList.count()) { QObject::disconnect(connection); return; }
    _objectList.insert(i, actualObject.data());
    if (!insertRows(i, 1) || !self) return;
    setDirty(true);
}

void QmlObjectListModel::insert(int i, QList<QObject*> objects)
{
    if (i < 0 || i > _objectList.count()) {
        qWarning() << "Invalid index index:count" << i << _objectList.count();
        return;
    }
    if (objects.isEmpty()) return;
    const QPointer<QmlObjectListModel> self(this);
    QList<QPointer<QObject>> actualObjects;
    for (QObject* object : objects) actualObjects.append(QPointer<QObject>(object));
    QList<QMetaObject::Connection> connections;
    const auto discardConnections = [&connections]() {
        for (const auto& connection : connections) QObject::disconnect(connection);
    };
    const auto inputsCurrent = [&]() {
        if (!self) return false;
        for (int index = 0; index < objects.size(); ++index)
            if (objects.at(index) && !actualObjects.at(index)) return false;
        return true;
    };
    int j = i;
    for (const auto& object: actualObjects) {
        if (object) {
            QQmlEngine::setObjectOwnership(object.data(), QQmlEngine::CppOwnership);
            if (!inputsCurrent()) { discardConnections(); return; }
            // Prepare inputs before inserting any prefix or announcing rows.
            const bool hasDirtySignal = object->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("dirtyChanged(bool)")) != -1;
            if (!inputsCurrent()) { discardConnections(); return; }
            if (hasDirtySignal && (!_skipDirtyFirstItem || j != 0)) {
                connections.append(QObject::connect(object.data(), SIGNAL(dirtyChanged(bool)), this, SLOT(_childDirtyChanged(bool))));
                if (!inputsCurrent()) { discardConnections(); return; }
            }
        }
        j++;
    }
    if (i > _objectList.count()) { discardConnections(); return; }
    j = i;
    for (const auto& object: actualObjects) _objectList.insert(j++, object.data());
    if (!insertRows(i, objects.count()) || !self) return;
    setDirty(true);
}

void QmlObjectListModel::append(QObject* object)
{
    insert(_objectList.count(), object);
}

void QmlObjectListModel::append(QList<QObject*> objects)
{
    insert(_objectList.count(), objects);
}

QObjectList QmlObjectListModel::swapObjectList(const QObjectList& newlist)
{
    const QPointer<QmlObjectListModel> self(this);
    QList<QPointer<QObject>> oldObjects, newObjects;
    for (QObject* object : _objectList) oldObjects.append(QPointer<QObject>(object));
    for (QObject* object : newlist) newObjects.append(QPointer<QObject>(object));
    const bool externalReset = _externalBeginResetModel;
    if (!externalReset) {
        beginResetModel();
        if (!self) return {};
    }
    _objectList.clear();
    for (const auto& object : newObjects) _objectList.append(object.data());
    if (!externalReset) {
        endResetModel();
        if (!self) return {};
        emit countChanged(count());
        if (!self) return {};
    }
    QObjectList oldlist;
    for (const auto& object : oldObjects) oldlist.append(object.data());
    return oldlist;
}

int QmlObjectListModel::count() const
{
    return rowCount();
}

void QmlObjectListModel::setDirty(bool dirty)
{
    if (_dirty != dirty) {
        const QPointer<QmlObjectListModel> self(this);
        _dirty = dirty;
        if (!dirty) {
            // Need to clear dirty from all children
            QList<QPointer<QObject>> objects;
            for (QObject* object : _objectList) objects.append(QPointer<QObject>(object));
            for(const auto& object: objects) {
                if (!object) continue;
                const bool hasDirtyProperty = object->property("dirty").isValid();
                if (!self) return;
                if (object && hasDirtyProperty) {
                    object->setProperty("dirty", false);
                    if (!self) return;
                }
            }
        }
        emit dirtyChanged(_dirty);
    }
}

void QmlObjectListModel::_childDirtyChanged(bool dirty)
{
    _dirty |= dirty;
    // We want to emit dirtyChanged even if the actual value of _dirty didn't change. It can be a useful
    // signal to know when a child has changed dirty state
    emit dirtyChanged(_dirty);
}

void QmlObjectListModel::deleteListAndContents()
{
    const QPointer<QmlObjectListModel> self(this);
    QList<QPointer<QObject>> objects;
    for (QObject* object : _objectList) objects.append(QPointer<QObject>(object));
    for (const auto& object : objects) {
        if (object) object->deleteLater();
        if (!self) return;
    }
    deleteLater();
}

void QmlObjectListModel::clearAndDeleteContents()
{
    const QPointer<QmlObjectListModel> self(this);
    QList<QPointer<QObject>> objects;
    for (QObject* object : _objectList) objects.append(QPointer<QObject>(object));
    const bool externalReset = _externalBeginResetModel;
    if (!externalReset) {
        beginResetModel();
        if (!self) return;
    }
    _objectList.clear();
    for (const auto& object : objects) {
        if (object) object->deleteLater();
        if (!self) return;
    }
    if (!externalReset) {
        endResetModel();
        if (!self) return;
        emit countChanged(count());
    }
}

void QmlObjectListModel::beginReset()
{
    if (_externalBeginResetModel) {
        qWarning() << "QmlObjectListModel::beginReset already set";
        return;
    }
    const QPointer<QmlObjectListModel> self(this);
    _externalBeginResetModel = true;
    beginResetModel();
    if (!self) return;
}

void QmlObjectListModel::endReset()
{
    if (!_externalBeginResetModel) {
        qWarning() << "QmlObjectListModel::endReset begin not set";
        return;
    }
    const QPointer<QmlObjectListModel> self(this);
    _externalBeginResetModel = false;
    endResetModel();
    if (!self) return;
}
