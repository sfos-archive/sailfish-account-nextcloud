/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2021 - 2023 Jolla Ltd.
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

#include "nextcloudimagesclient.h"
#include "syncer_p.h"
#include "networkrequestgenerator_p.h"
#include "networkreplyparser_p.h"
#include "logging.h"

// Buteo
#include <PluginCbInterface.h>
#include <ProfileEngineDefs.h>
#include <ProfileManager.h>


Buteo::ClientPlugin* NextcloudImagesClientLoader::createClientPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface* cbInterface)
{
    return new NextcloudImagesClient(pluginName, profile, cbInterface);
}


NextcloudImagesClient::NextcloudImagesClient(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
    : ClientPlugin(pluginName, profile, cbInterface)
    , m_syncer(0)
    , m_accountId(0)
{

    if (!lcNextcloudProtocol().isDebugEnabled()) {
        NetworkRequestGenerator::debugEnabled = true;
        NetworkReplyParser::debugEnabled = true;
    }
}

NextcloudImagesClient::~NextcloudImagesClient()
{
}

void NextcloudImagesClient::connectivityStateChanged(Sync::ConnectivityType type, bool state)
{
    qCDebug(lcNextcloud) << "Received connectivity change event:" << type << " changed to " << state;
    if (type == Sync::CONNECTIVITY_INTERNET && !state) {
        // we lost connectivity during sync.
        abortSync(Buteo::SyncResults::CONNECTION_ERROR);
    }
}

bool NextcloudImagesClient::init()
{
    QString accountIdString = iProfile.key(Buteo::KEY_ACCOUNT_ID);
    m_accountId = accountIdString.toInt();
    if (m_accountId == 0) {
        qCCritical(lcNextcloud) << "Nextcloud Images profile does not specify" << Buteo::KEY_ACCOUNT_ID;
        return false;
    }

    m_syncDirection = iProfile.syncDirection();
    m_conflictResPolicy = iProfile.conflictResolutionPolicy();

    if (!m_syncer) {
        m_syncer = new Syncer(this, &iProfile);
        connect(m_syncer, &Syncer::syncSucceeded,
                this, &NextcloudImagesClient::syncSucceeded);
        connect(m_syncer, &Syncer::syncFailed,
                this, &NextcloudImagesClient::syncFailed);
    }

    return true;
}

bool NextcloudImagesClient::uninit()
{
    delete m_syncer;
    m_syncer = 0;
    return true;
}

bool NextcloudImagesClient::startSync()
{
    if (m_accountId == 0) {
        return false;
    }

    m_syncer->startSync(m_accountId);
    return true;
}

void NextcloudImagesClient::syncSucceeded()
{
    syncFinished(Buteo::SyncResults::NO_ERROR, QString());
}

void NextcloudImagesClient::syncFailed()
{
    syncFinished(Buteo::SyncResults::INTERNAL_ERROR, QString());
}

void NextcloudImagesClient::abortSync(Buteo::SyncResults::MinorCode minorErrorCode)
{
    m_syncer->abortSync();
    syncFinished(minorErrorCode, QStringLiteral("Sync aborted"));
}

void NextcloudImagesClient::syncFinished(Buteo::SyncResults::MinorCode minorErrorCode, const QString &message)
{
    if (minorErrorCode == Buteo::SyncResults::NO_ERROR) {
        qCDebug(lcNextcloud) << "Nextcloud Images sync succeeded!" << message;
        m_results = Buteo::SyncResults(QDateTime::currentDateTimeUtc(),
                                       Buteo::SyncResults::SYNC_RESULT_SUCCESS,
                                       Buteo::SyncResults::NO_ERROR);
        emit success(getProfileName(), message);
    } else {
        qCCritical(lcNextcloud) << "Nextcloud Images sync failed:" << minorErrorCode << message;
        m_results = Buteo::SyncResults(iProfile.lastSuccessfulSyncTime(), // don't change the last sync time
                                       Buteo::SyncResults::SYNC_RESULT_FAILED,
                                       minorErrorCode);
        emit error(getProfileName(), message, minorErrorCode);
    }
}

Buteo::SyncResults NextcloudImagesClient::getSyncResults() const
{
    return m_results;
}

bool NextcloudImagesClient::cleanUp()
{
    // This function is called after the account has been deleted.
    QString accountIdString = iProfile.key(Buteo::KEY_ACCOUNT_ID);
    m_accountId = accountIdString.toInt();
    if (m_accountId == 0) {
        qCCritical(lcNextcloud) << "Nextcloud Images profile does not specify" << Buteo::KEY_ACCOUNT_ID;
        return false;
    }

    if (!m_syncer) {
        m_syncer = new Syncer(this, &iProfile);
    }

    m_syncer->purgeAccount(m_accountId);
    delete m_syncer;
    m_syncer = 0;

    return true;
}
