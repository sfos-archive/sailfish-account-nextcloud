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

#ifndef NEXTCLOUDSHARESERVICESTATUS_H
#define NEXTCLOUDSHARESERVICESTATUS_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QVector>

// sailfishaccounts
#include <accountauthenticator.h>

// libaccounts-qt5
#include <Accounts/Manager>
#include <SignOn/SessionData>
#include <SignOn/Error>

class NextcloudShareServiceStatus : public QObject
{
    Q_OBJECT

public:
    NextcloudShareServiceStatus(QObject *parent = 0);

    enum QueryStatusMode {
        PassiveMode = 0, // query account information but don't sign in
        SignInMode = 1   // query account information and sign in
    };
    void queryStatus(QueryStatusMode mode = SignInMode);

    struct AccountDetails {
        int accountId;
        QString providerName;
        QString serviceName;
        QString displayName;
        QString accessToken;
        QString username;
        QString password;
        QString serverAddress;
        QString webdavPath;
        bool ignoreSslErrors;
    };
    AccountDetails details(int index = 0) const;
    AccountDetails detailsByIdentifier(int accountIdentifier) const;
    int count() const;

    bool setCredentialsNeedUpdate(int accountId, const QString &serviceName);

Q_SIGNALS:
    void serviceReady();
    void serviceError(const QString &message);

private Q_SLOTS:
    void signInResponseHandler(int accountId, const QString &serviceName, const AccountAuthenticatorCredentials &credentials);
    void signInErrorHandler(int accountId, const QString &serviceName);

private:
    AccountAuthenticator *m_auth;
    QString m_serviceName;
    enum AccountDetailsState {
        Waiting,
        Populated,
        Error
    };
    void setAccountDetailsState(int accountId, AccountDetailsState state);
    QVector<AccountDetails> m_accountDetails;
    QHash<int, int> m_accountIdToDetailsIdx;
    QHash<int, AccountDetailsState> m_accountDetailsState;
};

#endif // NEXTCLOUDSHARESERVICESTATUS_H
