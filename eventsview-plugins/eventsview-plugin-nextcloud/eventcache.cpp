/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Nextcloud account package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#include "eventcache.h"

#include <QtCore/QDebug>
#include <QtQml/QQmlInfo>

namespace {

const int maximumSignOnRetries = 3;

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
                if (m_signOnFailCount[req.accountId] < maximumSignOnRetries) {
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

void NextcloudEventCache::signOnResponse(int accountId,
                                         const QString &serviceName,
                                         const AccountAuthenticatorCredentials &credentials)
{
    Q_UNUSED(serviceName)

    // we need both username+password, OR accessToken.
    if (!credentials.accessToken.isEmpty()) {
        m_accountIdAccessTokens.insert(accountId, credentials.accessToken);
    } else {
        m_accountIdCredentials.insert(accountId, qMakePair<QString, QString>(credentials.username, credentials.password));
    }

    m_pendingAccountRequests.removeAll(accountId);
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}

void NextcloudEventCache::signOnError(int accountId, const QString &serviceName, const QString &errorString)
{
    qWarning() << "NextcloudEventCache: sign-on failed for account:" << accountId
               << "service:" << serviceName
               << "error:" << errorString;
    m_pendingAccountRequests.removeAll(accountId);
    m_signOnFailCount[accountId] += 1;
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}
