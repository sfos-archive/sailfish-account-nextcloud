/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "eventmodel.h"
#include "eventcache.h"
#include "accountauthenticator_p.h"

#include <QtDebug>

#define MAX_RETRY_SIGNON 3

NextcloudEventCache::NextcloudEventCache(QObject *parent)
    : SyncCache::EventCache(parent)
{
    openDatabase(QStringLiteral("nextcloud"));
}

void NextcloudEventCache::openDatabase(const QString &)
{
    QObject *contextObject = new QObject(this);
    SyncCache::EventCache::openDatabase(QStringLiteral("nextcloud"));
    connect(this, &NextcloudEventCache::openDatabaseFinished,
            contextObject, [this, contextObject] {
        contextObject->deleteLater();
    });
    connect(this, &NextcloudEventCache::openDatabaseFailed,
            contextObject, [this, contextObject] (const QString &errorMessage) {
        contextObject->deleteLater();
        qWarning() << "NextcloudEventCache: failed to open database:" << errorMessage;
    });
}

void NextcloudEventCache::populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &)
{
    PendingRequest req;
    req.idempToken = idempToken;
    req.accountId = accountId;
    req.eventId = eventId;
    req.type = PopulateEventImageType;
    performRequest(req);
}

void NextcloudEventCache::performRequest(const NextcloudEventCache::PendingRequest &request)
{
    m_pendingRequests.append(request);
    performRequests();
}

void NextcloudEventCache::performRequests()
{
    QList<NextcloudEventCache::PendingRequest>::iterator it = m_pendingRequests.begin();
    while (it != m_pendingRequests.end()) {
        NextcloudEventCache::PendingRequest req = *it;
        if (m_accountIdAccessTokens.contains(req.accountId)
                || m_accountIdCredentials.contains(req.accountId)) {
            switch (req.type) {
                case PopulateEventImageType:
                        SyncCache::EventCache::populateEventImage(
                                req.idempToken, req.accountId, req.eventId,
                                templateRequest(req.accountId));
                        break;
            }
            it = m_pendingRequests.erase(it);
        } else {
            if (!m_pendingAccountRequests.contains(req.accountId)) {
                // trigger an account flow to get the credentials.
                if (m_signOnFailCount[req.accountId] < MAX_RETRY_SIGNON) {
                    m_pendingAccountRequests.append(req.accountId);
                    signIn(req.accountId);
                } else {
                    qWarning() << "NextcloudEventCache refusing to perform sign-on request for failing account:" << req.accountId;
                }
            } else {
                // nothing, waiting for asynchronous account flow to finish.
            }
            ++it;
        }
    }
}

QNetworkRequest NextcloudEventCache::templateRequest(int accountId) const
{
    QUrl templateUrl(QStringLiteral("https://localhost:8080/"));
    if (m_accountIdCredentials.contains(accountId)) {
        templateUrl.setUserName(m_accountIdCredentials.value(accountId).first);
        templateUrl.setPassword(m_accountIdCredentials.value(accountId).second);
    }
    QNetworkRequest templateRequest(templateUrl);
    if (m_accountIdAccessTokens.contains(accountId)) {
        templateRequest.setRawHeader(QString(QLatin1String("Authorization")).toUtf8(),
                                     QString(QLatin1String("Bearer ")).toUtf8() + m_accountIdAccessTokens.value(accountId).toUtf8());
    }
    return templateRequest;
}

void NextcloudEventCache::signIn(int accountId)
{
    if (!m_auth) {
        m_auth = new AccountAuthenticator(this);
        connect(m_auth, &AccountAuthenticator::signInCompleted,
                this, &NextcloudEventCache::signOnResponse);
        connect(m_auth, &AccountAuthenticator::signInError,
                this, &NextcloudEventCache::signOnError);
    }
    m_auth->signIn(accountId, QStringLiteral("nextcloud-posts"));
}

void NextcloudEventCache::signOnResponse(int accountId, const QString &serviceName,
                                         const QString &serverUrl, const QString &webdavPath,
                                         const QString &username, const QString &password,
                                         const QString &accessToken, bool ignoreSslErrors)
{
    Q_UNUSED(serviceName)
    Q_UNUSED(serverUrl)
    Q_UNUSED(webdavPath)
    Q_UNUSED(ignoreSslErrors)

    // we need both username+password, OR accessToken.
    if (!accessToken.isEmpty()) {
        m_accountIdAccessTokens.insert(accountId, accessToken);
    } else {
        m_accountIdCredentials.insert(accountId, qMakePair<QString, QString>(username, password));
    }

    m_pendingAccountRequests.removeAll(accountId);
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}

void NextcloudEventCache::signOnError(int accountId, const QString &serviceName)
{
    qWarning() << "NextcloudEventCache: sign-on failed for account:" << accountId
               << "service:" << serviceName;
    m_pendingAccountRequests.removeAll(accountId);
    m_signOnFailCount[accountId] += 1;
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------

NextcloudEventsModel::NextcloudEventsModel(QObject *parent)
    : QAbstractListModel(parent), m_deferLoad(false), m_eventCache(Q_NULLPTR)
    , m_accountId(0)
{
    qRegisterMetaType<SyncCache::Event>();
    qRegisterMetaType<QVector<SyncCache::Event> >();
}

void NextcloudEventsModel::classBegin()
{
    m_deferLoad = true;
}

void NextcloudEventsModel::componentComplete()
{
    m_deferLoad = false;
    if (m_eventCache) {
        loadData();
    }
}

QModelIndex NextcloudEventsModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && row < m_data.size()
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant NextcloudEventsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_data.size()) {
        return QVariant();
    }

    // TODO: if image path is requested but empty,
    //       call m_cache->populateEventImagePath(),
    //       and when it succeeds, emit dataChanged(row).
    switch (role) {
        case AccountIdRole:     return m_data[row].accountId;
        case EventIdRole:       return m_data[row].eventId;
        case EventSubjectRole:  return m_data[row].eventSubject;
        case EventTextRole:     return m_data[row].eventText;
        case EventUrlRole:      return m_data[row].eventUrl;
        case ImageUrlRole:      return m_data[row].imageUrl;
        case ImagePathRole:     return m_data[row].imagePath;
        case TimestampRole:     return m_data[row].timestamp;
        default:                return QVariant();
    }
}

int NextcloudEventsModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QHash<int, QByteArray> NextcloudEventsModel::roleNames() const
{
    static QHash<int, QByteArray> retn {
        { AccountIdRole,        "accountId" },
        { EventIdRole,          "eventId" },
        { EventSubjectRole,     "eventSubject" },
        { EventTextRole,        "eventText" },
        { EventUrlRole,         "eventUrl" },
        { ImageUrlRole,         "imageUrl" },
        { ImagePathRole,        "imagePath" },
        { TimestampRole,        "timestamp" },
    };

    return retn;
}

SyncCache::EventCache* NextcloudEventsModel::eventCache() const
{
    return m_eventCache;
}

void NextcloudEventsModel::setEventCache(SyncCache::EventCache *cache)
{
    if (m_eventCache == cache) {
        return;
    }

    if (m_eventCache) {
        disconnect(m_eventCache, 0, this, 0);
    }

    m_eventCache = cache;
    emit eventCacheChanged();

    if (!m_deferLoad) {
        loadData();
    }

    connect(m_eventCache, &SyncCache::EventCache::eventsStored,
            this, [this] (const QVector<SyncCache::Event> &events) {
        for (const SyncCache::Event &event : events) {
            bool foundEvent = false;
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Event &existing(m_data[row]);
                if (event.accountId == existing.accountId
                        && event.eventId == existing.eventId) {
                    m_data.replace(row, event);
                    emit dataChanged(index(row, 0, QModelIndex()), index(row, 0, QModelIndex()));
                    foundEvent = true;
                }
            }

            if (!foundEvent
                    && (event.accountId == accountId() || accountId() == 0)) {
                emit beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(event);
                emit endInsertRows();
                emit rowCountChanged();
            }
        }
    });

    connect(m_eventCache, &SyncCache::EventCache::eventsDeleted,
            this, [this] (const QVector<SyncCache::Event> &events) {
        for (const SyncCache::Event &event : events) {
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Event &existing(m_data[row]);
                if (event.accountId == existing.accountId
                        && event.eventId == existing.eventId) {
                    emit beginRemoveRows(QModelIndex(), row, row);
                    m_data.remove(row);
                    emit endRemoveRows();
                    emit rowCountChanged();
                }
            }
        }
    });
}

int NextcloudEventsModel::accountId() const
{
    return m_accountId;
}

void NextcloudEventsModel::setAccountId(int id)
{
    if (m_accountId == id) {
        return;
    }

    m_accountId = id;
    emit accountIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QVariantMap NextcloudEventsModel::at(int row) const
{
    QVariantMap retn;
    if (row < 0 || row >= rowCount()) {
        return retn;
    }

    const QHash<int, QByteArray> roles = roleNames();
    QHash<int, QByteArray>::const_iterator it = roles.constBegin();
    QHash<int, QByteArray>::const_iterator end = roles.constEnd();
    for ( ; it != end; it++) {
        retn.insert(QString::fromLatin1(it.value()), data(index(row, 0, QModelIndex()), it.key()));
    }
    return retn;
}

void NextcloudEventsModel::loadData()
{
    if (!m_eventCache) {
        return;
    }

    if (m_accountId <= 0) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_eventCache, &SyncCache::EventCache::requestEventsFinished,
            contextObject, [this, contextObject] (int accountId,
                                                  const QVector<SyncCache::Event> &events) {
        if (accountId != this->accountId()) {
            return;
        }
        contextObject->deleteLater();
        const int oldSize = m_data.size();
        if (m_data.size()) {
            emit beginRemoveRows(QModelIndex(), 0, m_data.size() - 1);
            m_data.clear();
            emit endRemoveRows();
        }
        if (events.size()) {
            emit beginInsertRows(QModelIndex(), 0, events.size() - 1);
            m_data = events;
            emit endInsertRows();
        }
        if (m_data.size() != oldSize) {
            emit rowCountChanged();
        }
    });
    connect(m_eventCache, &SyncCache::EventCache::requestEventsFailed,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &errorMessage) {
        if (accountId != this->accountId()) {
            return;
        }
        contextObject->deleteLater();
        qWarning() << "NextcloudEventsModel::loadData: failed:" << errorMessage;
    });
    m_eventCache->requestEvents(m_accountId);
}

// call this periodically to detect changes made to the database by another process.
// this would go away if we had daemonized storage rather than direct-file database access...
void NextcloudEventsModel::refresh()
{
    if (!m_eventCache) {
        return;
    }

    if (m_accountId <= 0) {
        return;
    }

    if (m_deferLoad) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_eventCache, &SyncCache::EventCache::requestEventsFinished,
            contextObject, [this, contextObject] (int accountId,
                                                  const QVector<SyncCache::Event> &events) {
        if (accountId != this->accountId()) {
            return;
        }
        contextObject->deleteLater();

        int row = 0;
        const QVector<SyncCache::Event> oldEvents = m_data;
        QVector<SyncCache::Event>::const_iterator newValuesIter(events.constBegin());
        QVector<SyncCache::Event>::const_iterator oldValuesIter(oldEvents.constBegin());
        while (newValuesIter != events.constEnd() || oldValuesIter != oldEvents.constEnd()) {
            bool bothNeedIncrement = newValuesIter != events.constEnd()
                    && oldValuesIter != oldEvents.constEnd()
                    && newValuesIter->eventId == oldValuesIter->eventId;
            bool newNeedsIncrement =
                    // if they both need incrementing, then newIter needs incrementing
                    bothNeedIncrement ? true
                    // if they are both not at end, and newIter.timestamp > oldIter.timestamp, then newIter needs incrementing
                    : ((newValuesIter != events.constEnd() &&
                        oldValuesIter != oldEvents.constEnd() &&
                        newValuesIter->timestamp > oldValuesIter->timestamp)
                    // if only newIter is not at end, then newIter needs incrementing
                    || (newValuesIter != events.constEnd() &&
                        oldValuesIter == oldEvents.constEnd()));

            if (bothNeedIncrement) {
                // the current event exists in both old + new.
                // this is either a row change, or an unchanged row (identical data)
                QVector<int> changedRoles;
                if (newValuesIter->eventSubject != oldValuesIter->eventSubject) {
                    changedRoles.append(static_cast<int>(EventSubjectRole));
                }
                if (newValuesIter->eventText != oldValuesIter->eventText) {
                    changedRoles.append(static_cast<int>(EventTextRole));
                }
                if (newValuesIter->eventUrl != oldValuesIter->eventUrl) {
                    changedRoles.append(static_cast<int>(EventUrlRole));
                }
                if (!changedRoles.isEmpty()) {
                    m_data.replace(row, *newValuesIter);
                    emit dataChanged(createIndex(row, 0), createIndex(row, 0), changedRoles);
                }
            } else if (newNeedsIncrement) {
                // the current key exists only in new.
                // this is a row addition.
                emit beginInsertRows(QModelIndex(), row, row);
                m_data.insert(row, *newValuesIter);
                emit endInsertRows();
            } else {
                // the current key exists only in old.
                // this is a row removal.
                emit beginRemoveRows(QModelIndex(), row, row);
                m_data.removeAt(row);
                emit endRemoveRows();
            }

            if (bothNeedIncrement) {
                newValuesIter++;
                oldValuesIter++;
                row++; // we changed (or ignored) a value at the previous row.
            } else if (newNeedsIncrement) {
                newValuesIter++;
                row++; // we added a new value at the previous row.
            } else {
                oldValuesIter++;
                // no need to increment or decrement row, as we removed the old value associated with that row number.
            }
        }

        if (oldEvents.size() != m_data.size()) {
            emit rowCountChanged();
        }
    });
    connect(m_eventCache, &SyncCache::EventCache::requestEventsFailed,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &errorMessage) {
        if (accountId != this->accountId()) {
            return;
        }
        contextObject->deleteLater();
        qWarning() << "NextcloudEventsModel::refresh: failed:" << errorMessage;
    });
    m_eventCache->requestEvents(m_accountId);
}

//-----------------------------------------------------------------------------

NextcloudEventImageDownloader::NextcloudEventImageDownloader(QObject *parent)
    : QObject(parent)
    , m_deferLoad(false)
    , m_eventCache(Q_NULLPTR)
    , m_accountId(0)
    , m_idempToken(0)
{
}

void NextcloudEventImageDownloader::classBegin()
{
    m_deferLoad = true;
}

void NextcloudEventImageDownloader::componentComplete()
{
    m_deferLoad = false;
    if (m_eventCache) {
        loadImage();
    }
}

SyncCache::EventCache* NextcloudEventImageDownloader::eventCache() const
{
    return m_eventCache;
}

void NextcloudEventImageDownloader::setEventCache(SyncCache::EventCache *cache)
{
    if (m_eventCache == cache) {
        return;
    }

    if (m_eventCache) {
        disconnect(m_eventCache, 0, this, 0);
    }

    m_eventCache = cache;
    emit eventCacheChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

int NextcloudEventImageDownloader::accountId() const
{
    return m_accountId;
}

void NextcloudEventImageDownloader::setAccountId(int id)
{
    if (m_accountId == id) {
        return;
    }

    m_accountId = id;
    emit accountIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QString NextcloudEventImageDownloader::eventId() const
{
    return m_eventId;
}
void NextcloudEventImageDownloader::setEventId(const QString &id)
{
    if (m_eventId == id) {
        return;
    }

    m_eventId = id;
    emit eventIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QUrl NextcloudEventImageDownloader::imagePath() const
{
    return m_imagePath;
}

void NextcloudEventImageDownloader::loadImage()
{
    if (!m_eventCache) {
        qWarning() << "No event cache, cannot load image";
        return;
    }

    m_idempToken = qHash(m_eventId);
    QObject *contextObject = new QObject(this);
    connect(m_eventCache, &SyncCache::EventCache::populateEventImageFinished,
            contextObject, [this, contextObject] (int idempToken, const QString &path) {
        if (m_idempToken == idempToken) {
            contextObject->deleteLater();
            const QUrl imagePath = QUrl::fromLocalFile(path);
            if (m_imagePath != imagePath) {
                m_imagePath = imagePath;
                emit imagePathChanged();
            }
        }
    });
    connect(m_eventCache, &SyncCache::EventCache::populateEventImageFailed,
            contextObject, [this, contextObject] (int idempToken, const QString &errorMessage) {
        if (m_idempToken == idempToken) {
            contextObject->deleteLater();
            qWarning() << "NextcloudEventImageDownloader::loadImage: failed:" << errorMessage;
        }
    });
    m_eventCache->populateEventImage(m_idempToken, m_accountId, m_eventId, QNetworkRequest());
}
