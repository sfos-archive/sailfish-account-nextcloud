/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "accountauthenticator_p.h"

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

AccountAuthenticator::AccountAuthenticator(QObject *parent)
    : QObject(parent)
{
}

AccountAuthenticator::~AccountAuthenticator()
{
    while (m_authData.size()) {
        AuthData authData = m_authData.takeFirst();
        authData.identity->destroySession(authData.authSession);
        authData.identity->deleteLater();
        authData.account->deleteLater();
    }
}

void AccountAuthenticator::signIn(int accountId, const QString &serviceName)
{
    Accounts::Account *account = Accounts::Account::fromId(&m_manager, accountId, this);
    if (!account) {
        qWarning() << "unable to load account" << accountId;
        emit signInError(accountId, serviceName);
        return;
    }

    // determine which service to sign in with.
    Accounts::Service srv;
    Accounts::ServiceList services = account->services();
    Q_FOREACH (const Accounts::Service &s, services) {
        if (s.name() == serviceName) {
            srv = s;
            break;
        }
    }

    if (!srv.isValid()) {
        qWarning() << "unable to find service" << serviceName
                   << "for account:" << accountId;
        account->deleteLater();
        emit signInError(accountId, serviceName);
        return;
    }

    // determine the remote URL from the account settings, and then sign in.
    account->selectService(srv);
    if (!account->enabled()) {
        qWarning() << "Service:" << srv.name() << "is not enabled for account:" << accountId;
        account->deleteLater();
        emit signInError(accountId, serviceName);
        return;
    }

    const bool ignoreSslErrors = account->value(QStringLiteral("ignore_ssl_errors")).toBool();
    const QString serverAddress = account->value(QStringLiteral("server_address")).toString();
    const QString webdavPath = account->value(QStringLiteral("webdav_path")).toString(); // optional, may be empty.
    if (serverAddress.isEmpty()) {
        qWarning() << "no valid server url setting in account" << accountId;
        account->deleteLater();
        emit signInError(accountId, serviceName);
        return;
    }

    SignOn::Identity *ident = account->credentialsId() > 0 ? SignOn::Identity::existingIdentity(account->credentialsId()) : 0;
    if (!ident) {
        qWarning() << "no valid credentials for account" << accountId;
        account->deleteLater();
        emit signInError(accountId, serviceName);
        return;
    }

    Accounts::AccountService accSrv(account, srv);
    QString method = accSrv.authData().method();
    QString mechanism = accSrv.authData().mechanism();
    SignOn::AuthSession *session = ident->createSession(method);
    if (!session) {
        qWarning() << "unable to create authentication session with account" << accountId;
        account->deleteLater();
        ident->deleteLater();
        emit signInError(accountId, serviceName);
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
    authData.serviceName = serviceName;
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

void AccountAuthenticator::signOnResponse(const SignOn::SessionData &response)
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
                emit signInCompleted(accountId, authData.serviceName, authData.serverAddress, authData.webdavPath, QString(), QString(), accessToken, authData.ignoreSslErrors);
            } else if (!username.isEmpty() && !password.isEmpty()) {
                emit signInCompleted(accountId, authData.serviceName, authData.serverAddress, authData.webdavPath, username, password, QString(), authData.ignoreSslErrors);
            } else {
                qWarning() << "authentication succeeded, but couldn't find valid credentials";
                emit signInError(accountId, authData.serviceName);
            }
            return;
        }
    }

    qWarning() << "Authentication succeeded but unknown auth session";
    emit signInError(accountId, QString());
}

void AccountAuthenticator::signOnError(const SignOn::Error &error)
{
    const int accountId = sender()->property("accountId").toInt();
    const int authDataSize = m_authData.size();
    for (int i = 0; i < authDataSize; ++i) {
        if (m_authData[i].accountId == accountId) {
            AuthData authData = m_authData.takeAt(i);
            authData.identity->destroySession(authData.authSession);
            authData.identity->deleteLater();
            authData.account->deleteLater();
            qWarning() << "authentication error:" << error.type() << ":" << error.message();
            emit signInError(accountId, authData.serviceName);
            return;
        }
    }

    qWarning() << "Unknown authentication session, authentication error:" << error.type() << ":" << error.message();
    emit signInError(accountId, QString());
}

void AccountAuthenticator::setCredentialsNeedUpdate(int accountId, const QString &serviceName)
{
    Accounts::Account *account = m_manager.account(accountId);
    if (account) {
        Accounts::ServiceList services = account->services();
        Q_FOREACH (const Accounts::Service &s, services) {
            if (s.serviceType().toLower() == serviceName) {
                account->setValue(QStringLiteral("CredentialsNeedUpdate"), QVariant::fromValue<bool>(true));
                account->setValue(QStringLiteral("CredentialsNeedUpdateFrom"), QVariant::fromValue<QString>(serviceName));
                account->selectService(Accounts::Service());
                account->syncAndBlock();
                break;
            }
        }
    }
}
