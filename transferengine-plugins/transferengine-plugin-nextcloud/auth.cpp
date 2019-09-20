/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "auth_p.h"

#include <QtDebug>

#ifdef USE_SAILFISHKEYPROVIDER
#include <sailfishkeyprovider.h>

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
#endif // USE_SAILFISHKEYPROVIDER

Auth::Auth(QObject *parent)
    : QObject(parent)
    , m_serviceName(QStringLiteral("nextcloud-sharing"))
{
}

Auth::~Auth()
{
    while (m_authData.size()) {
        AuthData authData = m_authData.takeFirst();
        authData.identity->destroySession(authData.authSession);
        authData.identity->deleteLater();
        authData.account->deleteLater();
    }
}

void Auth::signIn(int accountId)
{
    Accounts::Account *account = Accounts::Account::fromId(&m_manager, accountId, this);
    if (!account) {
        qWarning() << Q_FUNC_INFO << "unable to load account" << accountId;
        emit signInError(accountId);
        return;
    }

    // determine which service to sign in with.
    Accounts::Service srv;
    Accounts::ServiceList services = account->services();
    Q_FOREACH (const Accounts::Service &s, services) {
        if (s.name() == m_serviceName) {
            srv = s;
            break;
        }
    }

    if (!srv.isValid()) {
        qWarning() << Q_FUNC_INFO << "unable to find sharing service for account" << accountId;
        account->deleteLater();
        emit signInError(accountId);
        return;
    }

    // determine the remote URL from the account settings, and then sign in.
    account->selectService(srv);
    if (!account->enabled()) {
        qWarning() << Q_FUNC_INFO << "Service:" << srv.name() << "is not enabled for account:" << accountId;
        account->deleteLater();
        emit signInError(accountId);
        return;
    }

    const bool ignoreSslErrors = account->value(QStringLiteral("ignore_ssl_errors")).toBool();
    const QString serverAddress = account->value(QStringLiteral("server_address")).toString();
    const QString webdavPath = account->value(QStringLiteral("webdav_path")).toString(); // optional, may be empty.
    if (serverAddress.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "no valid server url setting in account" << accountId;
        account->deleteLater();
        emit signInError(accountId);
        return;
    }

    SignOn::Identity *ident = account->credentialsId() > 0 ? SignOn::Identity::existingIdentity(account->credentialsId()) : 0;
    if (!ident) {
        qWarning() << Q_FUNC_INFO << "no valid credentials for account" << accountId;
        account->deleteLater();
        emit signInError(accountId);
        return;
    }

    Accounts::AccountService accSrv(account, srv);
    QString method = accSrv.authData().method();
    QString mechanism = accSrv.authData().mechanism();
    SignOn::AuthSession *session = ident->createSession(method);
    if (!session) {
        qWarning() << Q_FUNC_INFO << "unable to create authentication session with account" << accountId;
        account->deleteLater();
        ident->deleteLater();
        emit signInError(accountId);
        return;
    }

    QString clientId;
    QString clientSecret;
    QString consumerKey;
    QString consumerSecret;
#ifdef USE_SAILFISHKEYPROVIDER
    QString providerName = account->providerName();
    clientId = skp_storedKey(providerName, QString(), QStringLiteral("client_id"));
    clientSecret = skp_storedKey(providerName, QString(), QStringLiteral("client_secret"));
    consumerKey = skp_storedKey(providerName, QString(), QStringLiteral("consumer_key"));
    consumerSecret = skp_storedKey(providerName, QString(), QStringLiteral("consumer_secret"));
#endif

    QVariantMap signonSessionData = accSrv.authData().parameters();
    signonSessionData.insert("UiPolicy", SignOn::NoUserInteractionPolicy);
    if (!clientId.isEmpty()) signonSessionData.insert("ClientId", clientId);
    if (!clientSecret.isEmpty()) signonSessionData.insert("ClientSecret", clientSecret);
    if (!consumerKey.isEmpty()) signonSessionData.insert("ConsumerKey", consumerKey);
    if (!consumerSecret.isEmpty()) signonSessionData.insert("ConsumerSecret", consumerSecret);

    connect(session, SIGNAL(response(SignOn::SessionData)),
            this, SLOT(signOnResponse(SignOn::SessionData)),
            Qt::UniqueConnection);
    connect(session, SIGNAL(error(SignOn::Error)),
            this, SLOT(signOnError(SignOn::Error)),
            Qt::UniqueConnection);

    AuthData authData;
    authData.accountId = accountId;
    authData.mechanism = mechanism;
    authData.signonSessionData = signonSessionData;
    authData.account = account;
    authData.identity = ident;
    authData.authSession = session;
    authData.serverAddress = serverAddress;
    authData.webdavPath = webdavPath;
    authData.ignoreSslErrors = ignoreSslErrors;
    m_authData.append(authData);

    session->setProperty("accountId", accountId);
    session->process(SignOn::SessionData(signonSessionData), mechanism);
}

void Auth::signOnResponse(const SignOn::SessionData &response)
{
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

    const int accountId = sender()->property("accountId").toInt();
    const int authDataSize = m_authData.size();
    for (int i = 0; i < authDataSize; ++i) {
        if (m_authData[i].accountId == accountId) {
            AuthData authData = m_authData.takeAt(i);
            authData.identity->destroySession(authData.authSession);
            authData.identity->deleteLater();
            authData.account->deleteLater();

            // we need both username+password, OR accessToken.
            if (!accessToken.isEmpty()) {
                emit signInCompleted(accountId, authData.serverAddress, authData.webdavPath, QString(), QString(), accessToken, authData.ignoreSslErrors);
            } else if (!username.isEmpty() && !password.isEmpty()) {
                emit signInCompleted(accountId, authData.serverAddress, authData.webdavPath, username, password, QString(), authData.ignoreSslErrors);
            } else {
                qWarning() << Q_FUNC_INFO << "authentication succeeded, but couldn't find valid credentials";
                emit signInError(accountId);
            }
            return;
        }
    }

    qWarning() << "Authentication succeeded but unknown auth session";
    emit signInError(accountId);
}

void Auth::signOnError(const SignOn::Error &error)
{
    const int accountId = sender()->property("accountId").toInt();
    const int authDataSize = m_authData.size();
    for (int i = 0; i < authDataSize; ++i) {
        if (m_authData[i].accountId == accountId) {
            AuthData authData = m_authData.takeAt(i);
            authData.identity->destroySession(authData.authSession);
            authData.identity->deleteLater();
            authData.account->deleteLater();
            break;
        }
    }

    qWarning() << Q_FUNC_INFO << "authentication error:" << error.type() << ":" << error.message();
    emit signInError(accountId);
    return;
}

void Auth::setCredentialsNeedUpdate(int accountId)
{
    Accounts::Account *account = m_manager.account(accountId);
    if (account) {
        Accounts::ServiceList services = account->services();
        Q_FOREACH (const Accounts::Service &s, services) {
            if (s.serviceType().toLower() == m_serviceName) {
                account->setValue(QStringLiteral("CredentialsNeedUpdate"), QVariant::fromValue<bool>(true));
                account->setValue(QStringLiteral("CredentialsNeedUpdateFrom"), QVariant::fromValue<QString>(m_serviceName));
                account->selectService(Accounts::Service());
                account->syncAndBlock();
                break;
            }
        }
    }
}
