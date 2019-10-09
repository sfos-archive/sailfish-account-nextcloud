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
#include "processmutex_p.h"

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
    EventImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent = Q_NULLPTR);
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
            EventImageDownloadWatcher *watcher = Q_NULLPTR);
    ~EventImageDownload();

    int m_idempToken = 0;
    QUrl m_imageUrl;
    QString m_fileName;
    QString m_eventId;
    QNetworkRequest m_templateRequest;
    QTimer *m_timeoutTimer = Q_NULLPTR;
    QNetworkReply *m_reply = Q_NULLPTR;
    QPointer<SyncCache::EventImageDownloadWatcher> m_watcher;
};

class EventImageDownloader : public QObject
{
    Q_OBJECT

public:
    EventImageDownloader(int maxActive = 4, QObject *parent = Q_NULLPTR);
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
    EventCacheThreadWorker(QObject *parent = Q_NULLPTR);
    ~EventCacheThreadWorker();

public Q_SLOTS:
    void openDatabase(const QString &accountType);

    void requestEvents(int accountId);

    void populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate);

Q_SIGNALS:
    void openDatabaseFailed(const QString &errorMessage);
    void openDatabaseFinished();

    void requestEventsFailed(int accountId, const QString &errorMessage);
    void requestEventsFinished(int accountId, const QVector<SyncCache::Event> &events);

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

    void requestEvents(int accountId);

    bool populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate);

private:
    QThread m_dbThread;
    EventCacheThreadWorker *m_worker;
};

class EventDatabasePrivate
{
public:
    mutable QScopedPointer<ProcessMutex> m_processMutex;
    QSqlDatabase m_database;
    bool m_inTransaction = false;
    QVector<QString> m_filesToDelete;
    QVector<SyncCache::Event> m_deletedEvents;
    QVector<SyncCache::Event> m_storedEvents;
};

} // namespace SyncCache

#endif // NEXTCLOUD_EVENTCACHE_P_H
