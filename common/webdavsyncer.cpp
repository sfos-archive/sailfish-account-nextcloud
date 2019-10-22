/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "webdavsyncer_p.h"
#include "accountauthenticator_p.h"
#include "networkreplyparser_p.h"

// buteo
#include <SyncProfile.h>
#include <LogMacros.h>

static const int HTTP_UNAUTHORIZED_ACCESS = 401;

WebDavSyncer::WebDavSyncer(QObject *parent, Buteo::SyncProfile *syncProfile, const QString &serviceName)
    : QObject(parent)
    , m_syncProfile(syncProfile)
    , m_serviceName(serviceName)
{
}

WebDavSyncer::~WebDavSyncer()
{
    delete m_auth;
}

void WebDavSyncer::abortSync()
{
    m_syncAborted = true;
}

void WebDavSyncer::startSync(int accountId)
{
    Q_ASSERT(accountId != 0);
    m_accountId = accountId;
    delete m_auth;
    m_auth = new AccountAuthenticator(this);
    connect(m_auth, &AccountAuthenticator::signInCompleted,
            this, &WebDavSyncer::sync);
    connect(m_auth, &AccountAuthenticator::signInError,
            this, &WebDavSyncer::signInError);
    LOG_DEBUG(Q_FUNC_INFO << "starting" << m_serviceName << "sync with account" << m_accountId);
    m_auth->signIn(accountId, m_serviceName);
}

void WebDavSyncer::signInError()
{
    emit syncFailed();
}

void WebDavSyncer::sync(int, const QString &, const QString &serverUrl, const QString &webdavPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors)
{
    m_serverUrl = serverUrl;
    m_webdavPath = webdavPath.isEmpty() ? QStringLiteral("/remote.php/webdav/") : webdavPath;
    m_username = username;
    m_password = password;
    m_accessToken = accessToken;
    m_ignoreSslErrors = ignoreSslErrors;

    beginSync();
}

void WebDavSyncer::finishWithHttpError(const QString &errorMessage, int httpCode)
{
    m_syncError = true;
    if (httpCode == HTTP_UNAUTHORIZED_ACCESS) {
        m_auth->setCredentialsNeedUpdate(m_accountId, m_serviceName);
    }
    finishWithError(QString("%1 (http status=%2)").arg(errorMessage).arg(httpCode));
}

void WebDavSyncer::finishWithError(const QString &errorMessage)
{
    LOG_WARNING("Nextcloud" << m_serviceName << "sync for account" << m_accountId << "finished with error:" << errorMessage);
    m_syncError = true;
    emit syncFailed();
}

void WebDavSyncer::finishWithSuccess()
{
    LOG_DEBUG(Q_FUNC_INFO << "Nextcloud" << m_serviceName << "sync with account" << m_accountId << "finished successfully!");
    emit syncSucceeded();
}
