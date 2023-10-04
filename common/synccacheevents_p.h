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

#ifndef NEXTCLOUD_SYNCCACHEEVENTS_P_H
#define NEXTCLOUD_SYNCCACHEEVENTS_P_H

#include "synccacheevents.h"
#include "synccachedatabase_p.h"

#include <QtCore/QVector>
#include <QtCore/QThread>

namespace SyncCache {

class EventImageDownloader;

class EventCacheThreadWorker : public QObject
{
    Q_OBJECT

public:
    EventCacheThreadWorker(QObject *parent = nullptr);
    ~EventCacheThreadWorker();

public Q_SLOTS:
    void openDatabase(const QString &accountType);

    void requestEvents(int accountId, bool includeLocallyDeleted);
    void deleteEvent(int accountId, const QString &eventId);
    void flagEventForDeletion(int accountId, const QString &eventId);

    void populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate);

Q_SIGNALS:
    void openDatabaseFailed(const QString &errorMessage);
    void openDatabaseFinished();

    void requestEventsFailed(int accountId, const QString &errorMessage);
    void requestEventsFinished(int accountId, const QVector<SyncCache::Event> &events);

    void deleteEventFailed(int accountId, const QString &eventId, const QString &errorMessage);
    void deleteEventFinished(int accountId, const QString &eventId);

    void flagEventForDeletionFailed(int accountId, const QString &eventId, const QString &errorMessage);
    void flagEventForDeletionFinished(int accountId, const QString &eventId);

    void populateEventImageFailed(int idempToken, const QString &errorMessage);
    void populateEventImageFinished(int idempToken, const QString &path);

    void eventsStored(const QVector<SyncCache::Event> &events);
    void eventsDeleted(const QVector<SyncCache::Event> &events);

private:
    EventDatabase m_db;
    EventImageDownloader *m_downloader;
};

class EventCachePrivate : public QObject
{
    Q_OBJECT

public:
    EventCachePrivate(EventCache *parent);
    ~EventCachePrivate();

Q_SIGNALS:
    void openDatabase(const QString &accountType);

    void requestEvents(int accountId, bool includeLocallyDeleted);
    void deleteEvent(int accountId, const QString &eventId);
    void flagEventForDeletion(int accountId, const QString &eventId);

    bool populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate);

private:
    QThread m_dbThread;
    EventCacheThreadWorker *m_worker;
};

class EventDatabasePrivate : public DatabasePrivate
{
public:
    EventDatabasePrivate(EventDatabase *parent);

    int currentSchemaVersion() const override;
    QVector<const char *> createStatements() const override;
    QVector<UpgradeOperation> upgradeVersions() const override;

    bool preTransactionCommit() override;
    void transactionCommittedPreUnlock() override;
    void transactionCommittedPostUnlock() override;
    void transactionRolledBackPreUnlocked() override;

private:
    friend class SyncCache::EventDatabase;
    EventDatabase *m_eventDbParent;
    QVector<QString> m_filesToDelete;
    QVector<SyncCache::Event> m_deletedEvents;
    QVector<SyncCache::Event> m_locallyDeletedEvents;
    QVector<SyncCache::Event> m_storedEvents;
    QVector<QString> m_tempFilesToDelete;
    QVector<SyncCache::Event> m_tempDeletedEvents;
    QVector<SyncCache::Event> m_tempLocallyDeletedEvents;
    QVector<SyncCache::Event> m_tempStoredEvents;
};

} // namespace SyncCache

#endif // NEXTCLOUD_SYNCCACHEEVENTS_P_H
