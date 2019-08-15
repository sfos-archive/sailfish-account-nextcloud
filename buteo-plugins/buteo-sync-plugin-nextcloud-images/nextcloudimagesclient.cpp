/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudimagesclient.h"
#include "syncer_p.h"

// Buteo
#include <PluginCbInterface.h>
#include <LogMacros.h>
#include <ProfileEngineDefs.h>
#include <ProfileManager.h>

extern "C" NextcloudImagesClient* createPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
{
    return new NextcloudImagesClient(pluginName, profile, cbInterface);
}

extern "C" void destroyPlugin(NextcloudImagesClient *client)
{
    delete client;
}

NextcloudImagesClient::NextcloudImagesClient(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
    : ClientPlugin(pluginName, profile, cbInterface)
    , m_syncer(0)
    , m_accountId(0)
{
}

NextcloudImagesClient::~NextcloudImagesClient()
{
}

void NextcloudImagesClient::connectivityStateChanged(Sync::ConnectivityType type, bool state)
{
    LOG_DEBUG("Received connectivity change event:" << type << " changed to " << state);
    if (type == Sync::CONNECTIVITY_INTERNET && !state) {
        // we lost connectivity during sync.
        abortSync(Sync::SYNC_CONNECTION_ERROR);
    }
}

bool NextcloudImagesClient::init()
{
    QString accountIdString = iProfile.key(Buteo::KEY_ACCOUNT_ID);
    m_accountId = accountIdString.toInt();
    if (m_accountId == 0) {
        LOG_CRITICAL("Nextcloud Images profile does not specify" << Buteo::KEY_ACCOUNT_ID);
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

void NextcloudImagesClient::abortSync(Sync::SyncStatus status)
{
    abort(status);
}

void NextcloudImagesClient::abort(Sync::SyncStatus status)
{
    m_syncer->abortSync();
    syncFinished(status, QStringLiteral("Sync aborted"));
}

void NextcloudImagesClient::syncFinished(int minorErrorCode, const QString &message)
{
    if (minorErrorCode == Buteo::SyncResults::NO_ERROR) {
        LOG_DEBUG("Nextcloud Images sync succeeded!" << message);
        m_results = Buteo::SyncResults(QDateTime::currentDateTimeUtc(),
                                       Buteo::SyncResults::SYNC_RESULT_SUCCESS,
                                       Buteo::SyncResults::NO_ERROR);
        emit success(getProfileName(), message);
    } else {
        LOG_CRITICAL("Nextcloud Images sync failed:" << minorErrorCode << message);
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
        LOG_CRITICAL("Nextcloud Images profile does not specify" << Buteo::KEY_ACCOUNT_ID);
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
