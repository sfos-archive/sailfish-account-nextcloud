/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "imagemodels.h"

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


NextcloudImageCache::NextcloudImageCache(QObject *parent)
    : SyncCache::ImageCache(parent)
{
    openDatabase(QStringLiteral("nextcloud"));
}

void NextcloudImageCache::openDatabase(const QString &)
{
    QObject *contextObject = new QObject(this);
    SyncCache::ImageCache::openDatabase(QStringLiteral("nextcloud"));
    connect(this, &NextcloudImageCache::openDatabaseFinished,
            contextObject, [this, contextObject] {
        contextObject->deleteLater();
    });
    connect(this, &NextcloudImageCache::openDatabaseFailed,
            contextObject, [this, contextObject] (const QString &errorMessage) {
        contextObject->deleteLater();
        qWarning() << "NextcloudImageCache: failed to open database:" << errorMessage;
    });
}

void NextcloudImageCache::populateUserThumbnail(int idempToken, int accountId, const QString &userId, const QNetworkRequest &)
{
    PendingRequest req;
    req.idempToken = idempToken;
    req.accountId = accountId;
    req.userId = userId;
    req.type = PopulateUserThumbnailType;
    performRequest(req);
}

void NextcloudImageCache::populateAlbumThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QNetworkRequest &)
{
    PendingRequest req;
    req.idempToken = idempToken;
    req.accountId = accountId;
    req.userId = userId;
    req.albumId = albumId;
    req.type = PopulateAlbumThumbnailType;
    performRequest(req);
}

void NextcloudImageCache::populatePhotoThumbnail(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &)
{
    PendingRequest req;
    req.idempToken = idempToken;
    req.accountId = accountId;
    req.userId = userId;
    req.albumId = albumId;
    req.photoId = photoId;
    req.type = PopulatePhotoThumbnailType;
    performRequest(req);
}

void NextcloudImageCache::populatePhotoImage(int idempToken, int accountId, const QString &userId, const QString &albumId, const QString &photoId, const QNetworkRequest &)
{
    PendingRequest req;
    req.idempToken = idempToken;
    req.accountId = accountId;
    req.userId = userId;
    req.albumId = albumId;
    req.photoId = photoId;
    req.type = PopulatePhotoImageType;
    performRequest(req);
}

void NextcloudImageCache::performRequest(const NextcloudImageCache::PendingRequest &request)
{
    m_pendingRequests.append(request);
    performRequests();
}

void NextcloudImageCache::performRequests()
{
    QList<NextcloudImageCache::PendingRequest>::iterator it = m_pendingRequests.begin();
    while (it != m_pendingRequests.end()) {
        NextcloudImageCache::PendingRequest req = *it;
        if (m_accountIdAccessTokens.contains(req.accountId)
                || m_accountIdCredentials.contains(req.accountId)) {
            switch (req.type) {
                case PopulateUserThumbnailType:
                        SyncCache::ImageCache::populateUserThumbnail(
                                req.idempToken, req.accountId, req.userId,
                                templateRequest(req.accountId));
                        break;
                case PopulateAlbumThumbnailType:
                        SyncCache::ImageCache::populateAlbumThumbnail(
                                req.idempToken, req.accountId, req.userId, req.albumId,
                                templateRequest(req.accountId));
                        break;
                case PopulatePhotoThumbnailType:
                        SyncCache::ImageCache::populatePhotoThumbnail(
                                req.idempToken, req.accountId, req.userId, req.albumId, req.photoId,
                                templateRequest(req.accountId));
                        break;
                case PopulatePhotoImageType:
                        SyncCache::ImageCache::populatePhotoImage(
                                req.idempToken, req.accountId, req.userId, req.albumId, req.photoId,
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
                    qWarning() << "NextcloudImageCache refusing to perform sign-on request for failing account:" << req.accountId;
                }
            } else {
                // nothing, waiting for asynchronous account flow to finish.
            }
            ++it;
        }
    }
}

QNetworkRequest NextcloudImageCache::templateRequest(int accountId) const
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

void NextcloudImageCache::signIn(int accountId)
{
    Accounts::Account *account = m_accounts.value(accountId);
    if (account == Q_NULLPTR) {
        account = Accounts::Account::fromId(&m_manager, accountId, this);
        m_accounts.insert(accountId, account);
    }
    if (account == Q_NULLPTR) {
        qWarning() << "NextcloudImageCache: unable to load account" << accountId;
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
        qWarning() << "NextcloudImageCache: unable to find sync service for account" << accountId;
        m_signOnFailCount[accountId] += 1;
        cleanUpAccount(accountId);
        QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
        return;
    }

    // determine the remote URL from the account settings, and then sign in.
    account->selectService(srv);
    if (!account->enabled()) {
        qWarning() << "NextcloudImageCache: sync service:" << srv.name() << "is not enabled for account:" << account->id();
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
        qWarning() << "NextcloudImageCache: no valid credentials for account" << accountId;
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
        qWarning() << "NextcloudImageCache: unable to create authentication session with account" << accountId;
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
            this, &NextcloudImageCache::signOnResponse,
            Qt::UniqueConnection);
    connect(session, &SignOn::AuthSession::error,
            this, &NextcloudImageCache::signOnError,
            Qt::UniqueConnection);

    session->setProperty("accountId", accountId);
    session->setProperty("mechanism", mechanism);
    session->setProperty("signonSessionData", signonSessionData);
    session->process(SignOn::SessionData(signonSessionData), mechanism);
}

void NextcloudImageCache::signOnResponse(const SignOn::SessionData &response)
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

void NextcloudImageCache::signOnError(const SignOn::Error &error)
{
    SignOn::AuthSession *session = qobject_cast<SignOn::AuthSession*>(sender());
    int accountId = session->property("accountId").toInt();
    qWarning() << "NextcloudImageCache: sign-on failed for account:" << accountId << ":" << error.type() << ":" << error.message();
    m_pendingAccountRequests.removeAll(accountId);
    m_signOnFailCount[accountId] += 1;
    cleanUpAccount(accountId, session);
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}

void NextcloudImageCache::cleanUpAccount(int accountId, SignOn::AuthSession *session)
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

NextcloudUsersModel::NextcloudUsersModel(QObject *parent)
    : QAbstractListModel(parent), m_deferLoad(false), m_imageCache(Q_NULLPTR)
{
    qRegisterMetaType<SyncCache::User>();
    qRegisterMetaType<QVector<SyncCache::User> >();
}

void NextcloudUsersModel::classBegin()
{
    m_deferLoad = true;
}

void NextcloudUsersModel::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadData();
    }
}

QModelIndex NextcloudUsersModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && row < m_data.size()
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant NextcloudUsersModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_data.size()) {
        return QVariant();
    }

    // TODO: if thumbnail path is requested but empty,
    //       call m_cache->populateUserThumbnail(),
    //       and when it succeeds, emit dataChanged(row).
    switch (role) {
        case AccountIdRole:     return m_data[row].accountId;
        case UserIdRole:        return m_data[row].userId;
        case ThumbnailUrlRole:  return m_data[row].thumbnailUrl;
        case ThumbnailPathRole: return m_data[row].thumbnailPath;
        default:                return QVariant();
    }
}

int NextcloudUsersModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QHash<int, QByteArray> NextcloudUsersModel::roleNames() const
{
    static QHash<int, QByteArray> retn {
        { AccountIdRole,        "accountId" },
        { UserIdRole,           "userId" },
        { ThumbnailUrlRole,     "thumbnailUrl" },
        { ThumbnailPathRole,    "thumbnailPath" }
    };

    return retn;
}

SyncCache::ImageCache* NextcloudUsersModel::imageCache() const
{
    return m_imageCache;
}

void NextcloudUsersModel::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    if (!m_deferLoad) {
        loadData();
    }

    connect(m_imageCache, &SyncCache::ImageCache::usersStored,
            this, [this] (const QVector<SyncCache::User> &users) {
        for (const SyncCache::User &user : users) {
            bool foundUser = false;
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::User &existing(m_data[row]);
                if (user.accountId == existing.accountId
                        && user.userId == existing.userId) {
                    m_data.replace(row, user);
                    emit dataChanged(index(row, 0, QModelIndex()), index(row, 0, QModelIndex()));
                    foundUser = true;
                }
            }

            if (!foundUser) {
                emit beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(user);
                emit endInsertRows();
                emit rowCountChanged();
            }
        }
    });

    connect(m_imageCache, &SyncCache::ImageCache::usersDeleted,
            this, [this] (const QVector<SyncCache::User> &users) {
        for (const SyncCache::User &user : users) {
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::User &existing(m_data[row]);
                if (user.accountId == existing.accountId
                        && user.userId == existing.userId) {
                    emit beginRemoveRows(QModelIndex(), row, row);
                    m_data.remove(row);
                    emit endRemoveRows();
                    emit rowCountChanged();
                }
            }
        }
    });
}

QVariantMap NextcloudUsersModel::at(int row) const
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

void NextcloudUsersModel::loadData()
{
    if (!m_imageCache) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_imageCache, &SyncCache::ImageCache::requestUsersFinished,
            contextObject, [this, contextObject] (const QVector<SyncCache::User> &users) {
        contextObject->deleteLater();
        const int oldSize = m_data.size();
        if (m_data.size()) {
            emit beginRemoveRows(QModelIndex(), 0, m_data.size() - 1);
            m_data.clear();
            emit endRemoveRows();
        }
        if (users.size()) {
            emit beginInsertRows(QModelIndex(), 0, users.size() - 1);
            m_data = users;
            emit endInsertRows();
        }
        if (m_data.size() != oldSize) {
            emit rowCountChanged();
        }
    });
    connect(m_imageCache, &SyncCache::ImageCache::requestUsersFailed,
            contextObject, [this, contextObject] (const QString &errorMessage) {
        contextObject->deleteLater();
        qWarning() << "NextcloudUsersModel::loadData: failed:" << errorMessage;
    });
    m_imageCache->requestUsers();
}

//-----------------------------------------------------------------------------

NextcloudAlbumsModel::NextcloudAlbumsModel(QObject *parent)
    : QAbstractListModel(parent), m_deferLoad(false), m_imageCache(Q_NULLPTR)
    , m_accountId(0)
{
    qRegisterMetaType<SyncCache::Album>();
    qRegisterMetaType<QVector<SyncCache::Album> >();
}

void NextcloudAlbumsModel::classBegin()
{
    m_deferLoad = true;
}

void NextcloudAlbumsModel::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadData();
    }
}

QModelIndex NextcloudAlbumsModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && row < m_data.size()
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant NextcloudAlbumsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_data.size()) {
        return QVariant();
    }

    // TODO: if thumbnail path is requested but empty,
    //       call m_cache->populateAlbumThumbnail(),
    //       and when it succeeds, emit dataChanged(row).
    switch (role) {
        case AccountIdRole:     return m_data[row].accountId;
        case UserIdRole:        return m_data[row].userId;
        case AlbumIdRole:       return m_data[row].albumId;
        case ParentAlbumIdRole: return m_data[row].parentAlbumId;
        case AlbumNameRole:     return m_data[row].albumName;
        case PhotoCountRole:    return m_data[row].photoCount;
        case ThumbnailUrlRole:  return m_data[row].thumbnailUrl;
        case ThumbnailPathRole: return m_data[row].thumbnailPath;
        default:                return QVariant();
    }
}

int NextcloudAlbumsModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QHash<int, QByteArray> NextcloudAlbumsModel::roleNames() const
{
    static QHash<int, QByteArray> retn {
        { AccountIdRole,        "accountId" },
        { UserIdRole,           "userId" },
        { AlbumIdRole,          "albumId" },
        { ParentAlbumIdRole,    "parentAlbumId" },
        { AlbumNameRole,        "albumName" },
        { PhotoCountRole,       "photoCount" },
        { ThumbnailUrlRole,     "thumbnailUrl" },
        { ThumbnailPathRole,    "thumbnailPath" },
    };

    return retn;
}

SyncCache::ImageCache* NextcloudAlbumsModel::imageCache() const
{
    return m_imageCache;
}

void NextcloudAlbumsModel::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    if (!m_deferLoad) {
        loadData();
    }

    connect(m_imageCache, &SyncCache::ImageCache::albumsStored,
            this, [this] (const QVector<SyncCache::Album> &albums) {
        for (const SyncCache::Album &album : albums) {
            bool foundAlbum = false;
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Album &existing(m_data[row]);
                if (album.accountId == existing.accountId
                        && album.userId == existing.userId
                        && album.albumId == existing.albumId) {
                    m_data.replace(row, album);
                    emit dataChanged(index(row, 0, QModelIndex()), index(row, 0, QModelIndex()));
                    foundAlbum = true;
                }
            }

            if (!foundAlbum
                    && (album.accountId == accountId() || accountId() == 0)
                    && (album.userId == userId() || userId().isEmpty())) {
                emit beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(album);
                emit endInsertRows();
                emit rowCountChanged();
            }
        }
    });

    connect(m_imageCache, &SyncCache::ImageCache::albumsDeleted,
            this, [this] (const QVector<SyncCache::Album> &albums) {
        for (const SyncCache::Album &album : albums) {
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Album &existing(m_data[row]);
                if (album.accountId == existing.accountId
                        && album.userId == existing.userId
                        && album.albumId == existing.albumId) {
                    emit beginRemoveRows(QModelIndex(), row, row);
                    m_data.remove(row);
                    emit endRemoveRows();
                    emit rowCountChanged();
                }
            }
        }
    });
}

int NextcloudAlbumsModel::accountId() const
{
    return m_accountId;
}

void NextcloudAlbumsModel::setAccountId(int id)
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

QString NextcloudAlbumsModel::userId() const
{
    return m_userId;
}

void NextcloudAlbumsModel::setUserId(const QString &id)
{
    if (m_userId == id) {
        return;
    }

    m_userId = id;
    emit userIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QVariantMap NextcloudAlbumsModel::at(int row) const
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

void NextcloudAlbumsModel::loadData()
{
    if (!m_imageCache) {
        return;
    }

    if (m_accountId <= 0) {
        return;
    }

    if (m_userId.isEmpty()) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_imageCache, &SyncCache::ImageCache::requestAlbumsFinished,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QVector<SyncCache::Album> &albums) {
        if (accountId != this->accountId()
                || userId != this->userId()) {
            return;
        }
        contextObject->deleteLater();
        const int oldSize = m_data.size();
        if (m_data.size()) {
            emit beginRemoveRows(QModelIndex(), 0, m_data.size() - 1);
            m_data.clear();
            emit endRemoveRows();
        }
        if (albums.size()) {
            emit beginInsertRows(QModelIndex(), 0, albums.size() - 1);
            m_data = albums;
            emit endInsertRows();
        }
        if (m_data.size() != oldSize) {
            emit rowCountChanged();
        }
    });
    connect(m_imageCache, &SyncCache::ImageCache::requestAlbumsFailed,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QString &errorMessage) {
        if (accountId != this->accountId()
                || userId != this->userId()) {
            return;
        }
        contextObject->deleteLater();
        qWarning() << "NextcloudAlbumsModel::loadData: failed:" << errorMessage;
    });
    m_imageCache->requestAlbums(m_accountId, m_userId);
}

//-----------------------------------------------------------------------------

NextcloudPhotosModel::NextcloudPhotosModel(QObject *parent)
    : QAbstractListModel(parent), m_deferLoad(false), m_imageCache(Q_NULLPTR)
    , m_accountId(0)
{
    qRegisterMetaType<SyncCache::Photo>();
    qRegisterMetaType<QVector<SyncCache::Photo> >();
}

void NextcloudPhotosModel::classBegin()
{
    m_deferLoad = true;
}

void NextcloudPhotosModel::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadData();
    }
}

QModelIndex NextcloudPhotosModel::index(int row, int column, const QModelIndex &parent) const
{
    return !parent.isValid() && column == 0 && row >= 0 && row < m_data.size()
            ? createIndex(row, column)
            : QModelIndex();
}

QVariant NextcloudPhotosModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (!index.isValid() || row < 0 || row >= m_data.size()) {
        return QVariant();
    }

    // TODO: if thumbnail path is requested but empty,
    //       call m_cache->populatePhotoThumbnail(),
    //       and when it succeeds, emit dataChanged(row).
    //       similarly for image path.
    switch (role) {
        case AccountIdRole:         return m_data[row].accountId;
        case UserIdRole:            return m_data[row].userId;
        case AlbumIdRole:           return m_data[row].albumId;
        case PhotoIdRole:           return m_data[row].photoId;
        case CreatedTimestampRole:  return m_data[row].createdTimestamp;
        case UpdatedTimestampRole:  return m_data[row].updatedTimestamp;
        case FileNameRole:          return m_data[row].fileName;
        case AlbumPathRole:         return m_data[row].albumPath;
        case DescriptionRole:       return m_data[row].description;
        case ThumbnailUrlRole:      return m_data[row].thumbnailUrl;
        case ThumbnailPathRole:     return m_data[row].thumbnailPath;
        case ImageUrlRole:          return m_data[row].imageUrl;
        case ImagePathRole:         return m_data[row].imagePath;
        case ImageWidthRole:        return m_data[row].imageWidth;
        case ImageHeightRole:       return m_data[row].imageHeight;
        default:                    return QVariant();
    }
}

int NextcloudPhotosModel::rowCount(const QModelIndex &) const
{
    return m_data.size();
}

QHash<int, QByteArray> NextcloudPhotosModel::roleNames() const
{
    static QHash<int, QByteArray> retn {
        { AccountIdRole,        "accountId" },
        { UserIdRole,           "userId" },
        { AlbumIdRole,          "albumId" },
        { PhotoIdRole,          "photoId" },
        { CreatedTimestampRole, "createdTimestamp" },
        { UpdatedTimestampRole, "updatedTimestamp" },
        { FileNameRole,         "fileName" },
        { AlbumPathRole,        "albumPath" },
        { DescriptionRole,      "description" },
        { ThumbnailUrlRole,     "thumbnailUrl" },
        { ThumbnailPathRole,    "thumbnailPath" },
        { ImageUrlRole,         "imageUrl" },
        { ImagePathRole,        "imagePath" },
        { ImageWidthRole,       "imageWidth" },
        { ImageHeightRole,      "imageHeight" },
    };

    return retn;
}

SyncCache::ImageCache* NextcloudPhotosModel::imageCache() const
{
    return m_imageCache;
}

void NextcloudPhotosModel::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    if (!m_deferLoad) {
        loadData();
    }

    connect(m_imageCache, &SyncCache::ImageCache::photosStored,
            this, [this] (const QVector<SyncCache::Photo> &photos) {
        for (const SyncCache::Photo &photo : photos) {
            bool foundPhoto = false;
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Photo &existing(m_data[row]);
                if (photo.accountId == existing.accountId
                        && photo.userId == existing.userId
                        && photo.albumId == existing.albumId
                        && photo.photoId == existing.photoId) {
                    m_data.replace(row, photo);
                    emit dataChanged(index(row, 0, QModelIndex()), index(row, 0, QModelIndex()));
                    foundPhoto = true;
                }
            }

            if (!foundPhoto
                    && (photo.accountId == accountId() || accountId() == 0)
                    && (photo.userId == userId() || userId().isEmpty())
                    && (photo.albumId == albumId() || albumId().isEmpty())) {
                emit beginInsertRows(QModelIndex(), m_data.size(), m_data.size());
                m_data.append(photo);
                emit endInsertRows();
                emit rowCountChanged();
            }
        }
    });

    connect(m_imageCache, &SyncCache::ImageCache::photosDeleted,
            this, [this] (const QVector<SyncCache::Photo> &photos) {
        for (const SyncCache::Photo &photo : photos) {
            for (int row = 0; row < m_data.size(); ++row) {
                const SyncCache::Photo &existing(m_data[row]);
                if (photo.accountId == existing.accountId
                        && photo.userId == existing.userId
                        && photo.albumId == existing.albumId
                        && photo.photoId == existing.photoId) {
                    emit beginRemoveRows(QModelIndex(), row, row);
                    m_data.remove(row);
                    emit endRemoveRows();
                    emit rowCountChanged();
                }
            }
        }
    });
}

int NextcloudPhotosModel::accountId() const
{
    return m_accountId;
}

void NextcloudPhotosModel::setAccountId(int id)
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

QString NextcloudPhotosModel::userId() const
{
    return m_userId;
}

void NextcloudPhotosModel::setUserId(const QString &id)
{
    if (m_userId == id) {
        return;
    }

    m_userId = id;
    emit userIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QString NextcloudPhotosModel::albumId() const
{
    return m_albumId;
}

void NextcloudPhotosModel::setAlbumId(const QString &id)
{
    if (m_albumId == id) {
        return;
    }

    m_albumId = id;
    emit albumIdChanged();

    if (!m_deferLoad) {
        loadData();
    }
}

QVariantMap NextcloudPhotosModel::at(int row) const
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

void NextcloudPhotosModel::loadData()
{
    if (!m_imageCache) {
        return;
    }

    if (m_accountId <= 0) {
        return;
    }

    if (m_userId.isEmpty()) {
        return;
    }

    QObject *contextObject = new QObject(this);
    connect(m_imageCache, &SyncCache::ImageCache::requestPhotosFinished,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QString &albumId,
                                                  const QVector<SyncCache::Photo> &photos) {
        if (accountId != this->accountId()
                || userId != this->userId()
                || albumId != this->albumId()) {
            return;
        }
        contextObject->deleteLater();
        const int oldSize = m_data.size();
        if (m_data.size()) {
            emit beginRemoveRows(QModelIndex(), 0, m_data.size() - 1);
            m_data.clear();
            emit endRemoveRows();
        }
        if (photos.size()) {
            emit beginInsertRows(QModelIndex(), 0, photos.size() - 1);
            m_data = photos;
            emit endInsertRows();
        }
        if (m_data.size() != oldSize) {
            emit rowCountChanged();
        }
    });
    connect(m_imageCache, &SyncCache::ImageCache::requestPhotosFailed,
            contextObject, [this, contextObject] (int accountId,
                                                  const QString &userId,
                                                  const QString &albumId,
                                                  const QString &errorMessage) {
        if (accountId != this->accountId()
                || userId != this->userId()
                || albumId != this->albumId()) {
            return;
        }
        contextObject->deleteLater();
        qWarning() << "NextcloudPhotosModel::loadData: failed:" << errorMessage;
    });
    m_imageCache->requestPhotos(m_accountId, m_userId, m_albumId);
}

//-----------------------------------------------------------------------------

NextcloudImageDownloader::NextcloudImageDownloader(QObject *parent)
    : QObject(parent)
    , m_deferLoad(false)
    , m_imageCache(Q_NULLPTR)
    , m_accountId(0)
    , m_downloadThumbnail(false)
    , m_downloadImage(false)
    , m_idempToken(0)
{
}

void NextcloudImageDownloader::classBegin()
{
    m_deferLoad = true;
}

void NextcloudImageDownloader::componentComplete()
{
    m_deferLoad = false;
    if (m_imageCache) {
        loadImage();
    }
}

SyncCache::ImageCache* NextcloudImageDownloader::imageCache() const
{
    return m_imageCache;
}

void NextcloudImageDownloader::setImageCache(SyncCache::ImageCache *cache)
{
    if (m_imageCache == cache) {
        return;
    }

    if (m_imageCache) {
        disconnect(m_imageCache, 0, this, 0);
    }

    m_imageCache = cache;
    emit imageCacheChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

int NextcloudImageDownloader::accountId() const
{
    return m_accountId;
}

void NextcloudImageDownloader::setAccountId(int id)
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

QString NextcloudImageDownloader::userId() const
{
    return m_userId;
}

void NextcloudImageDownloader::setUserId(const QString &id)
{
    if (m_userId == id) {
        return;
    }

    m_userId = id;
    emit userIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QString NextcloudImageDownloader::albumId() const
{
    return m_albumId;
}

void NextcloudImageDownloader::setAlbumId(const QString &id)
{
    if (m_albumId == id) {
        return;
    }

    m_albumId = id;
    emit albumIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QString NextcloudImageDownloader::photoId() const
{
    return m_photoId;
}
void NextcloudImageDownloader::setPhotoId(const QString &id)
{
    if (m_photoId == id) {
        return;
    }

    m_photoId = id;
    emit photoIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

bool NextcloudImageDownloader::downloadThumbnail() const
{
    return m_downloadThumbnail;
}

void NextcloudImageDownloader::setDownloadThumbnail(bool v)
{
    if (m_downloadThumbnail != v) {
        m_downloadThumbnail = v;
        emit downloadThumbnailChanged();
        loadImage();
    }
}

bool NextcloudImageDownloader::downloadImage() const
{
    return m_downloadImage;
}

void NextcloudImageDownloader::setDownloadImage(bool v)
{
    if (m_downloadImage != v) {
        m_downloadImage = v;
        emit downloadImageChanged();
        loadImage();
    }
}

QUrl NextcloudImageDownloader::imagePath() const
{
    return m_imagePath;
}

void NextcloudImageDownloader::loadImage()
{
    if (!m_imageCache) {
        qWarning() << "No image cache, cannot load image";
        return;
    }

    if (m_downloadImage) {
        m_idempToken = qHash(QStringLiteral("%1|%2|%3").arg(m_userId, m_albumId, m_photoId));
        QObject *contextObject = new QObject(this);
        connect(m_imageCache, &SyncCache::ImageCache::populatePhotoImageFinished,
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
        connect(m_imageCache, &SyncCache::ImageCache::populatePhotoImageFailed,
                contextObject, [this, contextObject] (int idempToken, const QString &errorMessage) {
            if (m_idempToken == idempToken) {
                contextObject->deleteLater();
                qWarning() << "NextcloudImageDownloader::loadImage: failed:" << errorMessage;
            }
        });
        m_imageCache->populatePhotoImage(m_idempToken, m_accountId, m_userId, m_albumId, m_photoId, QNetworkRequest());
    } else if (m_downloadThumbnail) {
        qWarning() << "NextcloudImageDownloader: Nextcloud doesn't provide thumbnails by default.";
    }
}
