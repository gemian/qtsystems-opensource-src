/****************************************************************************
**
** Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
** Copyright (C) 2015 Jolla.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSystems module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeinputdevicemodel_p.h"
#include "qinputinfo.h"

QT_BEGIN_NAMESPACE

QDeclarativeInputDeviceModel::QDeclarativeInputDeviceModel(QObject *parent) :
    QAbstractListModel(parent),
    deviceInfoManager(new QInputInfoManager),
    currentFilter(QInputDevice::UnknownType)
{
    connect(deviceInfoManager,SIGNAL(ready()),this,SLOT(updateDeviceList()));
    connect(deviceInfoManager,SIGNAL(filterChanged(QInputDevice::InputTypeFlags)),
            this,SLOT(updateDeviceList()));

    connect(deviceInfoManager, &QInputInfoManager::deviceAdded,
            this,&QDeclarativeInputDeviceModel::addedDevice);
    connect(deviceInfoManager, &QInputInfoManager::deviceRemoved,
            this,&QDeclarativeInputDeviceModel::removedDevice);
}

QDeclarativeInputDeviceModel::~QDeclarativeInputDeviceModel()
{
    delete deviceInfoManager;
}

QVariant QDeclarativeInputDeviceModel::data(const QModelIndex &index, int role) const
{
    QInputDevice *device = inputDevices.value(index.row());
    if (!device)
        return QVariant();

    switch (role) {
    case ServiceRole:
        return QVariant::fromValue(static_cast<QObject *>(device));
        break;
    case NameRole:
        return QVariant::fromValue(static_cast<QString>(device->name()));
        break;
    case IdentifierRole:
        return QVariant::fromValue(static_cast<QString>(device->identifier()));
        break;
    case ButtonsRole:
        return QVariant::fromValue(static_cast<QList <int> >(device->buttons()));
        break;
    case SwitchesRole:
        return QVariant::fromValue(static_cast<QList <int> >(device->switches()));
        break;
    case RelativeAxesRole:
        return QVariant::fromValue(static_cast<QList <int> >(device->relativeAxes()));
        break;
    case AbsoluteAxesRole:
        return QVariant::fromValue(static_cast<QList <int> >(device->absoluteAxes()));
        break;
    case TypesRole:
        return QVariant::fromValue(static_cast<int>(device->types()));
        break;
    };

    return QVariant();
}

int QDeclarativeInputDeviceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return inputDevices.count();
}

int QDeclarativeInputDeviceModel::indexOf(const QString &devicePath) const
{
    int idx(-1);
    Q_FOREACH (QInputDevice *device, inputDevices) {
        idx++;
        if (device->identifier() == devicePath) return idx;
    }

    return -1;
}

QInputDevice *QDeclarativeInputDeviceModel::get(int index) const
{
    if (index < 0 || index > inputDevices.count())
        return 0;
    return inputDevices.value(index);
}

void QDeclarativeInputDeviceModel::updateDeviceList()
{
    QList <QInputDevice *> newDevices = deviceInfoManager->deviceMap().values();
    int numNew = newDevices.count();

    for (int i = 0; i < numNew; i++) {
        int j = inputDevices.indexOf(newDevices.value(i));

        if (j == -1) {
            beginInsertRows(QModelIndex(), i, i);
            inputDevices.insert(i, newDevices.value(i));
            endInsertRows();
            Q_EMIT countChanged();
        } else if (i != j) {
            // changed its position -> move it
            QInputDevice* device = inputDevices.value(j);
            beginMoveRows(QModelIndex(), j, j, QModelIndex(), i);
            inputDevices.remove(j);
            if (i >= inputDevices.size())
                inputDevices.resize(i + 1);
            inputDevices.insert(i, device);
            endMoveRows();
        } //else {
        QModelIndex changedIndex(this->index(j, 0, QModelIndex()));
        Q_EMIT dataChanged(changedIndex, changedIndex);
    }

    int numOld = inputDevices.count();
    if (numOld > numNew) {
        beginRemoveRows(QModelIndex(), numNew, numOld - 1);
        inputDevices.remove(numNew, numOld - numNew);
        endRemoveRows();
        Q_EMIT countChanged();
    }
}

void QDeclarativeInputDeviceModel::addedDevice(QInputDevice *device)
{
    updateDeviceList();
    setFilter(currentFilter);
    Q_EMIT added(device);
}

void QDeclarativeInputDeviceModel::removedDevice(const QString &devicePath)
{
    updateDeviceList();
    setFilter(currentFilter);
    Q_EMIT removed(devicePath);
}

QHash<int,QByteArray> QDeclarativeInputDeviceModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[IdentifierRole] = "identifier";
    roles[ButtonsRole] = "buttons";
    roles[SwitchesRole] = "switches";
    roles[RelativeAxesRole] = "rAxis";
    roles[AbsoluteAxesRole] = "aAxis";
    roles[TypesRole] = "types";
    return roles;
}

/*
 * Returns the currently set device filter.
 * */
QInputDevice::InputTypeFlags QDeclarativeInputDeviceModel::filter()
{
    return currentFilter;
}

/*
 * Sets the current  input device filter to filter.
 * */
void QDeclarativeInputDeviceModel::setFilter(QInputDevice::InputTypeFlags filter)
{
    if (filter != currentFilter) {
        deviceInfoManager->setFilter(filter);
        currentFilter = filter;
        Q_EMIT filterChanged(filter);
    }
}

QT_END_NAMESPACE
