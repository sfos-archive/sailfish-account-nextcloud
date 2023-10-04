/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Nextcloud account package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#ifndef NEXTCLOUD_EVENTSVIEW_EVENTMODEL_H
#define NEXTCLOUD_EVENTSVIEW_EVENTMODEL_H

#include "synccacheevents.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QPair>
#include <QtQml/QQmlParserStatus>

class QTimer;
class MGConfItem;
class QDBusInterface;

class NextcloudEventModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::EventCache* eventCache READ eventCache WRITE setEventCache NOTIFY eventCacheChanged)
    Q_PROPERTY(int accountId READ accountId WRITE setAccountId NOTIFY accountIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(Actions supportedActions READ supportedActions NOTIFY supportedActionsChanged)

public:
    enum Action {
        DeleteEvent = 0x01,
        DeleteAllEvents = 0x02
    };
    Q_ENUM(Action)
    Q_DECLARE_FLAGS(Actions, Action)
    Q_FLAG(Actions)

    explicit NextcloudEventModel(QObject *parent = nullptr);
    ~NextcloudEventModel();

    // QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        EventIdRole,
        EventSubjectRole,
        EventTextRole,
        EventUrlRole,
        ImageUrlRole,
        ImagePathRole,
        TimestampRole,
        DeletedLocallyRole,
    };
    Q_ENUM(Roles)

    SyncCache::EventCache *eventCache() const;
    void setEventCache(SyncCache::EventCache *cache);

    int accountId() const;
    void setAccountId(int id);

    Actions supportedActions() const;

    Q_INVOKABLE QVariantMap at(int row) const;
    Q_INVOKABLE void refresh();

    Q_INVOKABLE void deleteEventAt(int row);
    Q_INVOKABLE void deleteAllEvents();

Q_SIGNALS:
    void eventCacheChanged();
    void accountIdChanged();
    void rowCountChanged();
    void supportedActionsChanged();

private:
    void loadData();
    void initRequests();
    void updateSupportedActions();

    void startNotificationDeleteTimer();
    void notificationDeleteTimeout();

    bool m_deferLoad = false;
    int m_accountId = 0;
    NextcloudEventModel::Actions m_supportedActions = 0;
    QVector<SyncCache::Event> m_data;
    SyncCache::EventCache *m_eventCache = nullptr;

    QTimer *m_notificationDeleteTimer = nullptr;
    QSet<QString> m_notificationsToDelete;

    MGConfItem *m_notifCapabilityConf = nullptr;
    QDBusInterface *m_buteoInterface = nullptr;
    QString m_buteoProfileId;
};

#endif // NEXTCLOUD_EVENTSVIEW_EVENTMODEL_H
