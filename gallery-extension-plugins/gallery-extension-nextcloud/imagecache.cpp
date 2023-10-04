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

#include "imagecache.h"

#include <QtCore/QDebug>
#include <QtQml/QQmlInfo>

namespace {

const int maximumSignOnRetries = 3;

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
                                templateRequest(req.accountId, true));
                        break;
                case PopulateAlbumThumbnailType:
                        SyncCache::ImageCache::populateAlbumThumbnail(
                                req.idempToken, req.accountId, req.userId, req.albumId,
                                templateRequest(req.accountId, true));
                        break;
                case PopulatePhotoThumbnailType:
                        SyncCache::ImageCache::populatePhotoThumbnail(
                                req.idempToken, req.accountId, req.userId, req.albumId, req.photoId,
                                templateRequest(req.accountId, true));
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
                if (m_signOnFailCount[req.accountId] < maximumSignOnRetries) {
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

QNetworkRequest NextcloudImageCache::templateRequest(int accountId, bool requiresBasicAuth) const
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
    } else if (requiresBasicAuth) {
        const QByteArray credentials((m_accountIdCredentials.value(accountId).first + ':' + m_accountIdCredentials.value(accountId).second).toUtf8());
        templateRequest.setRawHeader("Authorization", QByteArray("Basic ") + credentials.toBase64());
    }
    return templateRequest;
}

void NextcloudImageCache::signIn(int accountId)
{
    if (!m_auth) {
        m_auth = new AccountAuthenticator(this);
        connect(m_auth, &AccountAuthenticator::signInCompleted,
                this, &NextcloudImageCache::signOnResponse);
        connect(m_auth, &AccountAuthenticator::signInError,
                this, &NextcloudImageCache::signOnError);
    }
    m_auth->signIn(accountId, QStringLiteral("nextcloud-images"));
}

void NextcloudImageCache::signOnResponse(int accountId,
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

void NextcloudImageCache::signOnError(int accountId, const QString &serviceName, const QString &errorString)
{
    qWarning() << "NextcloudImageCache: sign-on failed for account:" << accountId
               << "service:" << serviceName
               << "error:" << errorString;
    m_pendingAccountRequests.removeAll(accountId);
    m_signOnFailCount[accountId] += 1;
    QMetaObject::invokeMethod(this, "performRequests", Qt::QueuedConnection);
}
