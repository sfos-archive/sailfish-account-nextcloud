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

#ifndef NEXTCLOUD_WEBDAVSYNCER_P_H
#define NEXTCLOUD_WEBDAVSYNCER_P_H

// sailfishaccounts
#include <accountauthenticator.h>

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class WebDavRequestGenerator;
class NetworkRequestGenerator;
namespace Buteo { class SyncProfile; }

class WebDavSyncer : public QObject
{
    Q_OBJECT

public:
    WebDavSyncer(QObject *parent, Buteo::SyncProfile *profile, const QString &serviceName);
   ~WebDavSyncer();

    void startSync(int accountId);
    virtual void abortSync();
    virtual void purgeAccount(int accountId) = 0;

    int accountId() const { return m_accountId; }
    QString serverUrl() const { return m_serverUrl; }
    QString webDavPath() const { return m_webdavPath; }

Q_SIGNALS:
    void syncSucceeded();
    void syncFailed();

protected Q_SLOTS:
    void sync(int accountId, const QString &serviceName, const AccountAuthenticatorCredentials &credentials);
    void signInError(int accountId, const QString &serviceName, const QString &errorString);

protected:
    virtual void beginSync() = 0;
    virtual void cleanUp() {}
    void finishWithHttpError(const QString &errorMessage, int httpCode);
    void finishWithError(const QString &errorMessage);
    void finishWithSuccess();

    Buteo::SyncProfile *m_syncProfile = nullptr;
    AccountAuthenticator *m_auth = nullptr;
    NetworkRequestGenerator *m_requestGenerator = nullptr;
    QNetworkAccessManager m_qnam;
    bool m_syncAborted = false;
    bool m_syncError = false;

    // auth related
    int m_accountId = 0;
    QString m_serviceName;
    QString m_serverUrl;
    QString m_webdavPath;
    QString m_username;
    QString m_password;
    QString m_accessToken;
    bool m_ignoreSslErrors = false;
};

#endif // NEXTCLOUD_WEBDAVSYNCER_P_H
