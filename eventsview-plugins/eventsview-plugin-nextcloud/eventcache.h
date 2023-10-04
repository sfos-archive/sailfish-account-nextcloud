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

#ifndef NEXTCLOUD_EVENTSVIEW_EVENTCACHE_H
#define NEXTCLOUD_EVENTSVIEW_EVENTCACHE_H

#include "synccacheevents.h"

// sailfishaccounts
#include <accountauthenticator.h>

#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtNetwork/QNetworkRequest>

class NextcloudEventCache : public SyncCache::EventCache
{
    Q_OBJECT
public:
    explicit NextcloudEventCache(QObject *parent = nullptr);

    void openDatabase(const QString &) override;
    void populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &) override;

    enum PendingRequestType {
        PopulateEventImageType
    };

    struct PendingRequest {
        PendingRequestType type;
        int idempToken;
        int accountId;
        QString eventId;
    };

private Q_SLOTS:
    void performRequests();
    void signOnResponse(int accountId, const QString &serviceName, const AccountAuthenticatorCredentials &credentials);
    void signOnError(int accountId, const QString &serviceName, const QString &errorString);

private:
    QList<PendingRequest> m_pendingRequests;
    QList<int> m_pendingAccountRequests;
    QHash<int, int> m_signOnFailCount;
    QHash<int, QPair<QString, QString> > m_accountIdCredentials;
    QHash<int, QString> m_accountIdAccessTokens;
    AccountAuthenticator *m_auth = nullptr;

    void signIn(int accountId);
    void performRequest(const PendingRequest &request);
    QNetworkRequest templateRequest(int accountId) const;
};

Q_DECLARE_METATYPE(NextcloudEventCache::PendingRequest)
Q_DECLARE_TYPEINFO(NextcloudEventCache::PendingRequest, Q_MOVABLE_TYPE);

#endif // NEXTCLOUD_EVENTSVIEW_EVENTCACHE_H
