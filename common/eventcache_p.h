/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_EVENTCACHE_P_H
#define NEXTCLOUD_EVENTCACHE_P_H

#include "eventcache.h"
#include "synccachedatabase_p.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVector>
#include <QtCore/QQueue>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QScopedPointer>
#include <QtCore/QPointer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtSql/QSqlDatabase>

namespace SyncCache {

class EventImageDownloadWatcher : public QObject
{
    Q_OBJECT

public:
    EventImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent = nullptr);
    ~EventImageDownloadWatcher();

    int idempToken() const;
    QUrl imageUrl() const;

Q_SIGNALS:
    void downloadFailed(const QString &errorMessage);
    void downloadFinished(const QUrl &filePath);

private:
    int m_idempToken;
    QUrl m_imageUrl;
};

class EventImageDownload
{
public:
    EventImageDownload(
            int idempToken = 0,
            const QUrl &imageUrl = QUrl(),
            const QString &fileName = QString(),
            const QString &eventId = QString(),
            const QNetworkRequest &templateRequest = QNetworkRequest(QUrl()),
            EventImageDownloadWatcher *watcher = nullptr);
    ~EventImageDownload();

    int m_idempToken = 0;
    QUrl m_imageUrl;
    QString m_fileName;
    QString m_eventId;
    QNetworkRequest m_templateRequest;
    QTimer *m_timeoutTimer = nullptr;
    QNetworkReply *m_reply = nullptr;
    QPointer<SyncCache::EventImageDownloadWatcher> m_watcher;
};

class EventImageDownloader : public QObject
{
    Q_OBJECT

public:
    EventImageDownloader(int maxActive = 4, QObject *parent = nullptr);
    ~EventImageDownloader();

    void setImageDirectory(const QString &path);
    EventImageDownloadWatcher *downloadImage(
            int idempToken,
            const QUrl &imageUrl,
            const QString &fileName,
            const QString &eventId,
            const QNetworkRequest &requestTemplate);

private Q_SLOTS:
    void triggerDownload();

private:
    void eraseActiveDownload(EventImageDownload *download);

    QNetworkAccessManager m_qnam;
    QQueue<EventImageDownload*> m_pending;
    QQueue<EventImageDownload*> m_active;
    int m_maxActive;
    QString m_imageDirectory;
};

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

    int currentSchemaVersion() const Q_DECL_OVERRIDE;
    QVector<const char *> createStatements() const Q_DECL_OVERRIDE;
    QVector<UpgradeOperation> upgradeVersions() const Q_DECL_OVERRIDE;

    void preTransactionCommit() Q_DECL_OVERRIDE;
    void transactionCommittedPreUnlock() Q_DECL_OVERRIDE;
    void transactionCommittedPostUnlock() Q_DECL_OVERRIDE;
    void transactionRolledBackPreUnlocked() Q_DECL_OVERRIDE;

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

#endif // NEXTCLOUD_EVENTCACHE_P_H
