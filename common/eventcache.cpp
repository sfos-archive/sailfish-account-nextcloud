/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "eventcache.h"
#include "eventcache_p.h"

#include <QtCore/QThread>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>

using namespace SyncCache;

namespace {
    QString privilegedEventsDirPath() {
        return QStringLiteral("%1/system/privileged/Posts").arg(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
    }

    QString filenameForEvent(const QString &eventId) {
        // parse out the image file name from the given event url.
        return eventId.isEmpty() ? eventId : QStringLiteral("eventImage");
    }
}

//-----------------------------------------------------------------------------

EventImageDownloadWatcher::EventImageDownloadWatcher(int idempToken, const QUrl &imageUrl, QObject *parent)
    : QObject(parent), m_idempToken(idempToken), m_imageUrl(imageUrl)
{
}

EventImageDownloadWatcher::~EventImageDownloadWatcher()
{
}

int EventImageDownloadWatcher::idempToken() const
{
    return m_idempToken;
}

QUrl EventImageDownloadWatcher::imageUrl() const
{
    return m_imageUrl;
}

//-----------------------------------------------------------------------------

EventImageDownload::EventImageDownload(
        int idempToken,
        const QUrl &imageUrl,
        const QString &fileName,
        const QString &eventId,
        const QNetworkRequest &templateRequest,
        EventImageDownloadWatcher *watcher)
    : m_idempToken(idempToken)
    , m_imageUrl(imageUrl)
    , m_fileName(fileName)
    , m_eventId(eventId)
    , m_templateRequest(templateRequest)
    , m_watcher(watcher)
{
    m_timeoutTimer = new QTimer;
}

EventImageDownload::~EventImageDownload()
{
    m_timeoutTimer->deleteLater();
    if (m_reply) {
        m_reply->deleteLater();
    }
}

//-----------------------------------------------------------------------------

EventImageDownloader::EventImageDownloader(int maxActive, QObject *parent)
    : QObject(parent), m_maxActive(maxActive > 0 && maxActive < 20 ? maxActive : 4), m_imageDirectory(QStringLiteral("Nextcloud"))
{
}

EventImageDownloader::~EventImageDownloader()
{
}

void EventImageDownloader::setImageDirectory(const QString &path)
{
    m_imageDirectory = path;
}

EventImageDownloadWatcher *EventImageDownloader::downloadImage(int idempToken, const QUrl &imageUrl, const QString &fileName, const QString &eventId, const QNetworkRequest &templateRequest)
{
    EventImageDownloadWatcher *watcher = new EventImageDownloadWatcher(idempToken, imageUrl, this);
    EventImageDownload *download = new EventImageDownload(idempToken, imageUrl, fileName, eventId, templateRequest, watcher);
    m_pending.enqueue(download);
    QMetaObject::invokeMethod(this, "triggerDownload", Qt::QueuedConnection);
    return watcher; // caller takes ownership.
}

void EventImageDownloader::triggerDownload()
{
    while (m_active.size() && (m_active.head() == nullptr || !m_active.head()->m_timeoutTimer->isActive())) {
        delete m_active.dequeue();
    }

    while (m_active.size() < m_maxActive && m_pending.size()) {
        EventImageDownload *download = m_pending.dequeue();
        m_active.enqueue(download);

        connect(download->m_timeoutTimer, &QTimer::timeout, this, [this, download] {
            if (download->m_watcher) {
                emit download->m_watcher->downloadFailed(QStringLiteral("Event Image download timed out"));
            }
            eraseActiveDownload(download);
        });

        download->m_timeoutTimer->setInterval(20 * 1000);
        download->m_timeoutTimer->setSingleShot(true);
        download->m_timeoutTimer->start();

        QNetworkRequest request(download->m_templateRequest);
        QUrl imageUrl(download->m_imageUrl);
        imageUrl.setUserName(download->m_templateRequest.url().userName());
        imageUrl.setPassword(download->m_templateRequest.url().password());
        request.setUrl(imageUrl);
        QNetworkReply *reply = m_qnam.get(request);
        download->m_reply = reply;
        if (reply) {
            connect(reply, &QNetworkReply::finished, this, [this, reply, download] {
                if (reply->error() != QNetworkReply::NoError) {
                    if (download->m_watcher) {
                        emit download->m_watcher->downloadFailed(QStringLiteral("Event Image download error: %1").arg(reply->errorString()));
                    }
                } else {
                    // save the file to the appropriate location.
                    const QByteArray replyData = reply->readAll();
                    if (replyData.isEmpty()) {
                        if (download->m_watcher) {
                            emit download->m_watcher->downloadFailed(QStringLiteral("Empty image data received, aborting"));
                        }
                    } else {
                        // const QString eventId = download->m_eventId; // XXX TODO: use this?  subdirectory for this post/event?
                        const QString filename = download->m_fileName;
                        const QString filepath = QStringLiteral("%1/%2/%3")
                                .arg(privilegedEventsDirPath(), m_imageDirectory, filename);
                        QDir dir = QFileInfo(filepath).dir();
                        if (!dir.exists()) {
                            dir.mkpath(QStringLiteral("."));
                        }
                        QFile file(filepath);
                        if (!file.open(QFile::WriteOnly)) {
                            if (download->m_watcher) {
                                emit download->m_watcher->downloadFailed(QStringLiteral("Error opening event image file %1 for writing: %2")
                                                                                   .arg(filepath, file.errorString()));
                            }
                        } else {
                            file.write(replyData);
                            file.close();
                            emit download->m_watcher->downloadFinished(filepath);
                        }
                    }
                }

                eraseActiveDownload(download);
            });
        }
    }
}

void EventImageDownloader::eraseActiveDownload(EventImageDownload *download)
{
    QQueue<EventImageDownload*>::iterator it = m_active.begin();
    QQueue<EventImageDownload*>::iterator end = m_active.end();
    for ( ; it != end; ++it) {
        if (*it == download) {
            m_active.erase(it);
            break;
        }
    }

    delete download;

    QMetaObject::invokeMethod(this, "triggerDownload", Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------

EventCacheThreadWorker::EventCacheThreadWorker(QObject *parent)
    : QObject(parent), m_downloader(nullptr)
{
}

EventCacheThreadWorker::~EventCacheThreadWorker()
{
}

void EventCacheThreadWorker::openDatabase(const QString &accountType)
{
    DatabaseError error;
    m_db.openDatabase(
            QStringLiteral("%1/%2.db").arg(privilegedEventsDirPath(), accountType),
            &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit openDatabaseFailed(error.errorMessage);
    } else {
        connect(&m_db, &EventDatabase::eventsStored,
                this, &EventCacheThreadWorker::eventsStored);
        connect(&m_db, &EventDatabase::eventsDeleted,
                this, &EventCacheThreadWorker::eventsDeleted);
        emit openDatabaseFinished();
    }
}

void EventCacheThreadWorker::requestEvents(int accountId, bool includeLocallyDeleted)
{
    DatabaseError error;
    QVector<SyncCache::Event> events = m_db.events(accountId, &error, includeLocallyDeleted);
    if (error.errorCode != DatabaseError::NoError) {
        emit requestEventsFailed(accountId, error.errorMessage);
    } else {
        emit requestEventsFinished(accountId, events);
    }
}

void EventCacheThreadWorker::deleteEvent(int accountId, const QString &eventId)
{
    DatabaseError error;
    m_db.deleteEvent(accountId, eventId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit deleteEventFailed(accountId, eventId, error.errorMessage);
    } else {
        emit deleteEventFinished(accountId, eventId);
    }
}

void EventCacheThreadWorker::flagEventForDeletion(int accountId, const QString &eventId)
{
    DatabaseError error;
    m_db.flagEventForDeletion(accountId, eventId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit flagEventForDeletionFailed(accountId, eventId, error.errorMessage);
    } else {
        emit flagEventForDeletionFinished(accountId, eventId);
    }
}

void EventCacheThreadWorker::populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate)
{
    DatabaseError error;
    Event event = m_db.event(accountId, eventId, &error);
    if (error.errorCode != DatabaseError::NoError) {
        emit populateEventImageFailed(idempToken, QStringLiteral("Error occurred while reading event info from db: %1").arg(error.errorMessage));
        return;
    }

    // the image already exists.
    const QString imagePath = event.imagePath.toString();
    if (!event.imagePath.isEmpty() && QFile::exists(imagePath)) {
        emit populateEventImageFinished(idempToken, imagePath);
        return;
    }

    // download the image.
    if (event.imageUrl.isEmpty()) {
        emit populateEventImageFailed(idempToken, QStringLiteral("Empty image url specified for event"));
        return;
    }
    if (!m_downloader) {
        m_downloader = new EventImageDownloader(4, this);
    }
    EventImageDownloadWatcher *watcher = m_downloader->downloadImage(idempToken, event.imageUrl, filenameForEvent(event.eventId), event.eventId, requestTemplate);
    connect(watcher, &EventImageDownloadWatcher::downloadFailed, this, [this, watcher, idempToken] (const QString &errorMessage) {
        emit populateEventImageFailed(idempToken, errorMessage);
        watcher->deleteLater();
    });
    connect(watcher, &EventImageDownloadWatcher::downloadFinished, this, [this, watcher, event, idempToken] (const QUrl &filePath) {
        // the file has been downloaded to disk.  attempt to update the database.
        DatabaseError storeError;
        Event eventToStore;
        eventToStore.accountId = event.accountId;
        eventToStore.eventId = event.eventId;
        eventToStore.eventSubject = event.eventSubject;
        eventToStore.eventText = event.eventText;
        eventToStore.eventUrl = event.eventUrl;
        eventToStore.imageUrl = event.imageUrl;
        eventToStore.imagePath = filePath;
        eventToStore.timestamp = event.timestamp;
        m_db.storeEvent(eventToStore, &storeError);
        if (storeError.errorCode != DatabaseError::NoError) {
            QFile::remove(filePath.toString());
            emit populateEventImageFailed(idempToken, storeError.errorMessage);
        } else {
            emit populateEventImageFinished(idempToken, filePath.toString());
        }
        watcher->deleteLater();
    });
}

//-----------------------------------------------------------------------------

EventCachePrivate::EventCachePrivate(EventCache *parent)
    : QObject(parent), m_worker(new EventCacheThreadWorker)
{
    qRegisterMetaType<SyncCache::Event>();
    qRegisterMetaType<QVector<SyncCache::Event> >();

    m_worker->moveToThread(&m_dbThread);
    connect(&m_dbThread, &QThread::finished, m_worker, &QObject::deleteLater);

    connect(this, &EventCachePrivate::openDatabase, m_worker, &EventCacheThreadWorker::openDatabase);
    connect(this, &EventCachePrivate::requestEvents, m_worker, &EventCacheThreadWorker::requestEvents);
    connect(this, &EventCachePrivate::deleteEvent, m_worker, &EventCacheThreadWorker::deleteEvent);
    connect(this, &EventCachePrivate::flagEventForDeletion, m_worker, &EventCacheThreadWorker::flagEventForDeletion);
    connect(this, &EventCachePrivate::populateEventImage, m_worker, &EventCacheThreadWorker::populateEventImage);

    connect(m_worker, &EventCacheThreadWorker::openDatabaseFailed, parent, &EventCache::openDatabaseFailed);
    connect(m_worker, &EventCacheThreadWorker::openDatabaseFinished, parent, &EventCache::openDatabaseFinished);
    connect(m_worker, &EventCacheThreadWorker::requestEventsFailed, parent, &EventCache::requestEventsFailed);
    connect(m_worker, &EventCacheThreadWorker::requestEventsFinished, parent, &EventCache::requestEventsFinished);
    connect(m_worker, &EventCacheThreadWorker::deleteEventFailed, parent, &EventCache::deleteEventFailed);
    connect(m_worker, &EventCacheThreadWorker::deleteEventFinished, parent, &EventCache::deleteEventFinished);
    connect(m_worker, &EventCacheThreadWorker::flagEventForDeletionFailed, parent, &EventCache::flagEventForDeletionFailed);
    connect(m_worker, &EventCacheThreadWorker::flagEventForDeletionFinished, parent, &EventCache::flagEventForDeletionFinished);

    connect(m_worker, &EventCacheThreadWorker::populateEventImageFailed, parent, &EventCache::populateEventImageFailed);
    connect(m_worker, &EventCacheThreadWorker::populateEventImageFinished, parent, &EventCache::populateEventImageFinished);

    connect(m_worker, &EventCacheThreadWorker::eventsStored, parent, &EventCache::eventsStored);
    connect(m_worker, &EventCacheThreadWorker::eventsDeleted, parent, &EventCache::eventsDeleted);

    m_dbThread.start();
    m_dbThread.setPriority(QThread::IdlePriority);
}

EventCachePrivate::~EventCachePrivate()
{
    m_dbThread.quit();
    m_dbThread.wait();
}

//-----------------------------------------------------------------------------

EventCache::EventCache(QObject *parent)
    : QObject(parent), d_ptr(new EventCachePrivate(this))
{
}

EventCache::~EventCache()
{
}

void EventCache::openDatabase(const QString &databaseFile)
{
    Q_D(EventCache);
    emit d->openDatabase(databaseFile);
}

void EventCache::requestEvents(int accountId, bool includeLocallyDeleted)
{
    Q_D(EventCache);
    emit d->requestEvents(accountId, includeLocallyDeleted);
}

void EventCache::deleteEvent(int accountId, const QString &eventId)
{
    Q_D(EventCache);
    emit d->deleteEvent(accountId, eventId);
}

void EventCache::flagEventForDeletion(int accountId, const QString &eventId)
{
    Q_D(EventCache);
    emit d->flagEventForDeletion(accountId, eventId);
}

void EventCache::populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &requestTemplate)
{
    Q_D(EventCache);
    emit d->populateEventImage(idempToken, accountId, eventId, requestTemplate);
}
