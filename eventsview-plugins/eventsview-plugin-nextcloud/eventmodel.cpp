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

#include <sailfishkeyprovider.h>

#include <QtDebug>

#define MAX_RETRY_SIGNON 3

namespace {
    QString skp_storedKey(const QString &provider, const QString &service, const QString &key)
    {
        QString retn;
        char *value = NULL;
        int success = SailfishKeyProvider_storedKey(provider.toLatin1(), service.toLatin1(), key.toLatin1(), &value);
        if (value) {
            if (success == 0) {
                retn = QString::fromLatin1(value);
            }
            free(value);
        }
        return retn;
    }
}


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
    Accounts::Account *account = m_accounts.value(accountId);
    if (account == Q_NULLPTR) {
        account = Accounts::Account::fromId(&m_manager, accountId, this);
        m_accounts.insert(accountId, account);
    }
    if (account == Q_NULLPTR) {
        qWarning() << "NextcloudEventCache: unable to load account" << accountId;
        m_signOnFailCount[accountId] += 1;
        cleanUpAccount(accountId);
        QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
        return;
    }

    // determine which service to sign in with.
    Accounts::Service srv;
    Accounts::ServiceList services = account->services();
    Q_FOREACH (const Accounts::Service &s, services) {
        if (s.serviceType().toLower() == QStringLiteral("sync")) {
            srv = s;
            break;
        }
    }

    if (!srv.isValid()) {
        qWarning() << "NextcloudEventCache: unable to find sync service for account" << accountId;
        m_signOnFailCount[accountId] += 1;
        cleanUpAccount(accountId);
        QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
        return;
    }

    // determine the remote URL from the account settings, and then sign in.
    account->selectService(srv);
    if (!account->enabled()) {
        qWarning() << "NextcloudEventCache: sync service:" << srv.name() << "is not enabled for account:" << account->id();
        m_signOnFailCount[accountId] += 1;
        cleanUpAccount(accountId);
        QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
        return;
    }

    SignOn::Identity *ident = m_identities.value(accountId);
    if (ident == Q_NULLPTR) {
        ident = account->credentialsId() > 0 ? SignOn::Identity::existingIdentity(account->credentialsId()) : 0;
        m_identities.insert(accountId, ident);
    }

    if (ident == Q_NULLPTR) {
        qWarning() << "NextcloudEventCache: no valid credentials for account" << accountId;
        m_signOnFailCount[accountId] += 1;
        cleanUpAccount(accountId);
        QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
        return;
    }

    Accounts::AccountService accSrv(account, srv);
    QString method = accSrv.authData().method();
    QString mechanism = accSrv.authData().mechanism();
    SignOn::AuthSession *session = ident->createSession(method);
    if (!session) {
        qWarning() << "NextcloudEventCache: unable to create authentication session with account" << accountId;
        m_signOnFailCount[accountId] += 1;
        cleanUpAccount(accountId);
        QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
        return;
    }

    QString providerName = account->providerName();
    QString clientId;
    QString clientSecret;
    QString consumerKey;
    QString consumerSecret;

    clientId = skp_storedKey(providerName, QString(), QStringLiteral("client_id"));
    clientSecret = skp_storedKey(providerName, QString(), QStringLiteral("client_secret"));
    consumerKey = skp_storedKey(providerName, QString(), QStringLiteral("consumer_key"));
    consumerSecret = skp_storedKey(providerName, QString(), QStringLiteral("consumer_secret"));

    QVariantMap signonSessionData = accSrv.authData().parameters();
    signonSessionData.insert("UiPolicy", SignOn::NoUserInteractionPolicy);
    if (!clientId.isEmpty()) signonSessionData.insert("ClientId", clientId);
    if (!clientSecret.isEmpty()) signonSessionData.insert("ClientSecret", clientSecret);
    if (!consumerKey.isEmpty()) signonSessionData.insert("ConsumerKey", consumerKey);
    if (!consumerSecret.isEmpty()) signonSessionData.insert("ConsumerSecret", consumerSecret);

    connect(session, &SignOn::AuthSession::response,
            this, &NextcloudEventCache::signOnResponse,
            Qt::UniqueConnection);
    connect(session, &SignOn::AuthSession::error,
            this, &NextcloudEventCache::signOnError,
            Qt::UniqueConnection);

    session->setProperty("accountId", accountId);
    session->setProperty("mechanism", mechanism);
    session->setProperty("signonSessionData", signonSessionData);
    session->process(SignOn::SessionData(signonSessionData), mechanism);
}

void NextcloudEventCache::signOnResponse(const SignOn::SessionData &response)
{
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    QString username, password, accessToken;
    Q_FOREACH (const QString &key, response.propertyNames()) {
        if (key.toLower() == QStringLiteral("username")) {
            username = response.getProperty(key).toString();
        } else if (key.toLower() == QStringLiteral("secret")) {
            password = response.getProperty(key).toString();
        } else if (key.toLower() == QStringLiteral("password")) {
            password = response.getProperty(key).toString();
        } else if (key.toLower() == QStringLiteral("accesstoken")) {
            accessToken = response.getProperty(key).toString();
        }
    }

    // we need both username+password, OR accessToken.
    int accountId = session->property("accountId").toInt();
    if (!accessToken.isEmpty()) {
        m_accountIdAccessTokens.insert(accountId, accessToken);
    } else {
        m_accountIdCredentials.insert(accountId, qMakePair<QString, QString>(username, password));
    }

    m_pendingAccountRequests.removeAll(accountId);
    cleanUpAccount(accountId, session);
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}

void NextcloudEventCache::signOnError(const SignOn::Error &error)
{
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    int accountId = session->property("accountId").toInt();
    qWarning() << "NextcloudEventCache: sign-on failed for account:" << accountId << ":" << error.type() << ":" << error.message();
    m_pendingAccountRequests.removeAll(accountId);
    m_signOnFailCount[accountId] += 1;
    cleanUpAccount(accountId, session);
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}

void NextcloudEventCache::cleanUpAccount(int accountId, SignOn::AuthSession *session)
{
    Accounts::Account *account = m_accounts.take(accountId);
    SignOn::Identity *ident = m_identities.take(accountId);

    if (account) {
        account->deleteLater();
    }

    if (ident) {
        ident->deleteLater();
        if (session) {
            ident->destroySession(session);
        }
    }
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
