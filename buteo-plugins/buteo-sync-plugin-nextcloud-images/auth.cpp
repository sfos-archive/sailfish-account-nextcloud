/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "auth_p.h"
#include <LogMacros.h>

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
    , m_account(0)
    , m_ident(0)
    , m_session(0)
    , m_ignoreSslErrors(false)
{
}

Auth::~Auth()
{
    delete m_account;
    if (m_ident && m_session) {
        m_ident->destroySession(m_session);
    }
    delete m_ident;
}

void Auth::signIn(int accountId)
{
    m_account = Accounts::Account::fromId(&m_manager, accountId, this);
    if (!m_account) {
        LOG_WARNING(Q_FUNC_INFO << "unable to load account" << accountId);
        emit signInError();
        return;
    }

    // determine which service to sign in with.
    Accounts::Service srv;
    Accounts::ServiceList services = m_account->services();
    Q_FOREACH (const Accounts::Service &s, services) {
        if (s.serviceType().toLower() == QStringLiteral("sync")
                && s.name() == QStringLiteral("nextcloud-images")) {
            srv = s;
            break;
        }
    }

    if (!srv.isValid()) {
        LOG_WARNING(Q_FUNC_INFO << "unable to find carddav service for account" << accountId);
        emit signInError();
        return;
    }

    // determine the remote URL from the account settings, and then sign in.
    m_account->selectService(srv);
    if (!m_account->enabled()) {
        LOG_WARNING("Service:" << srv.name() << "is not enabled for account:" << m_account->id());
        emit signInError();
        return;
    }
    m_ignoreSslErrors = m_account->value("ignore_ssl_errors").toBool();
    m_serverUrl = m_account->value("server_address").toString();
    m_webdavPath = m_account->value("webdav_path").toString(); // optional, may be empty.
    if (m_serverUrl.isEmpty()) {
        LOG_WARNING(Q_FUNC_INFO << "no valid server url setting in account" << accountId);
        emit signInError();
        return;
    }

    m_ident = m_account->credentialsId() > 0 ? SignOn::Identity::existingIdentity(m_account->credentialsId()) : 0;
    if (!m_ident) {
        LOG_WARNING(Q_FUNC_INFO << "no valid credentials for account" << accountId);
        emit signInError();
        return;
    }

    Accounts::AccountService accSrv(m_account, srv);
    QString method = accSrv.authData().method();
    QString mechanism = accSrv.authData().mechanism();
    SignOn::AuthSession *session = m_ident->createSession(method);
    if (!session) {
        LOG_WARNING(Q_FUNC_INFO << "unable to create authentication session with account" << accountId);
        emit signInError();
        return;
    }

    QString providerName = m_account->providerName();
    QString clientId;
    QString clientSecret;
    QString consumerKey;
    QString consumerSecret;
#ifdef USE_SAILFISHKEYPROVIDER
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

    session->setProperty("accountId", accountId);
    session->setProperty("mechanism", mechanism);
    session->setProperty("signonSessionData", signonSessionData);
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

    // we need both username+password, OR accessToken.
    if (!accessToken.isEmpty()) {
        emit signInCompleted(m_serverUrl, m_webdavPath, QString(), QString(), accessToken, m_ignoreSslErrors);
    } else if (!username.isEmpty() && !password.isEmpty()) {
        emit signInCompleted(m_serverUrl, m_webdavPath, username, password, QString(), m_ignoreSslErrors);
    } else {
        LOG_WARNING(Q_FUNC_INFO << "authentication succeeded, but couldn't find valid credentials");
        emit signInError();
    }
}

void Auth::signOnError(const SignOn::Error &error)
{
    LOG_WARNING(Q_FUNC_INFO << "authentication error:" << error.type() << ":" << error.message());
    emit signInError();
    return;
}

void Auth::setCredentialsNeedUpdate(int accountId)
{
    Accounts::Account *account = m_manager.account(accountId);
    if (account) {
        Accounts::ServiceList services = account->services();
        Q_FOREACH (const Accounts::Service &s, services) {
            if (s.serviceType().toLower() == QStringLiteral("carddav")) {
                account->setValue(QStringLiteral("CredentialsNeedUpdate"), QVariant::fromValue<bool>(true));
                account->setValue(QStringLiteral("CredentialsNeedUpdateFrom"), QVariant::fromValue<QString>(QString::fromLatin1("carddav-sync")));
                account->selectService(Accounts::Service());
                account->syncAndBlock();
                break;
            }
        }
    }
}
