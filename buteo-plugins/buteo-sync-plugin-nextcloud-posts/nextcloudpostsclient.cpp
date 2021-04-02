/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Copyright (C) 2021 Jolla Ltd.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudpostsclient.h"
#include "syncer_p.h"
#include "networkrequestgenerator_p.h"
#include "networkreplyparser_p.h"

// Buteo
#include <PluginCbInterface.h>
#include <LogMacros.h>
#include <ProfileEngineDefs.h>
#include <ProfileManager.h>

Buteo::ClientPlugin* NextcloudPostsClientLoader::createClientPlugin(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface* cbInterface)
{
    return new NextcloudPostsClient(pluginName, profile, cbInterface);
}


NextcloudPostsClient::NextcloudPostsClient(
        const QString& pluginName,
        const Buteo::SyncProfile& profile,
        Buteo::PluginCbInterface *cbInterface)
    : ClientPlugin(pluginName, profile, cbInterface)
    , m_syncer(0)
    , m_accountId(0)
{
    if (Buteo::Logger::instance()->getLogLevel() >= 7) {
        NetworkRequestGenerator::debugEnabled = true;
        NetworkReplyParser::debugEnabled = true;
    }
}

NextcloudPostsClient::~NextcloudPostsClient()
{
}

void NextcloudPostsClient::connectivityStateChanged(Sync::ConnectivityType type, bool state)
{
    LOG_DEBUG("Received connectivity change event:" << type << " changed to " << state);
    if (type == Sync::CONNECTIVITY_INTERNET && !state) {
        // we lost connectivity during sync.
        abortSync(Sync::SYNC_CONNECTION_ERROR);
    }
}

bool NextcloudPostsClient::init()
{
    QString accountIdString = iProfile.key(Buteo::KEY_ACCOUNT_ID);
    m_accountId = accountIdString.toInt();
    if (m_accountId == 0) {
        LOG_CRITICAL("Nextcloud Posts profile does not specify" << Buteo::KEY_ACCOUNT_ID);
        return false;
    }

    m_syncDirection = iProfile.syncDirection();
    m_conflictResPolicy = iProfile.conflictResolutionPolicy();

    if (!m_syncer) {
        m_syncer = new Syncer(this, &iProfile);
        connect(m_syncer, &Syncer::syncSucceeded,
                this, &NextcloudPostsClient::syncSucceeded);
        connect(m_syncer, &Syncer::syncFailed,
                this, &NextcloudPostsClient::syncFailed);
    }

    return true;
}

bool NextcloudPostsClient::uninit()
{
    delete m_syncer;
    m_syncer = 0;
    return true;
}

bool NextcloudPostsClient::startSync()
{
    if (m_accountId == 0) {
        return false;
    }

    m_syncer->startSync(m_accountId);
    return true;
}

void NextcloudPostsClient::syncSucceeded()
{
    syncFinished(Buteo::SyncResults::NO_ERROR, QString());
}

void NextcloudPostsClient::syncFailed()
{
    syncFinished(Buteo::SyncResults::INTERNAL_ERROR, QString());
}

void NextcloudPostsClient::abortSync(Sync::SyncStatus status)
{
    abort(status);
}

void NextcloudPostsClient::abort(Sync::SyncStatus status)
{
    m_syncer->abortSync();
    syncFinished(status, QStringLiteral("Sync aborted"));
}

void NextcloudPostsClient::syncFinished(int minorErrorCode, const QString &message)
{
    if (minorErrorCode == Buteo::SyncResults::NO_ERROR) {
        LOG_DEBUG("Nextcloud Posts sync succeeded!" << message);
        m_results = Buteo::SyncResults(QDateTime::currentDateTimeUtc(),
                                       Buteo::SyncResults::SYNC_RESULT_SUCCESS,
                                       Buteo::SyncResults::NO_ERROR);
        emit success(getProfileName(), message);
    } else {
        LOG_CRITICAL("Nextcloud Posts sync failed:" << minorErrorCode << message);
        m_results = Buteo::SyncResults(iProfile.lastSuccessfulSyncTime(), // don't change the last sync time
                                       Buteo::SyncResults::SYNC_RESULT_FAILED,
                                       minorErrorCode);
        emit error(getProfileName(), message, minorErrorCode);
    }
}

Buteo::SyncResults NextcloudPostsClient::getSyncResults() const
{
    return m_results;
}

bool NextcloudPostsClient::cleanUp()
{
    // This function is called after the account has been deleted.
    QString accountIdString = iProfile.key(Buteo::KEY_ACCOUNT_ID);
    m_accountId = accountIdString.toInt();
    if (m_accountId == 0) {
        LOG_CRITICAL("Nextcloud Posts profile does not specify" << Buteo::KEY_ACCOUNT_ID);
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
