/****************************************************************************************
**
** Copyright (c) 2020 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudbackupoperationclient.h"
#include "syncer_p.h"
#include "networkrequestgenerator_p.h"
#include "networkreplyparser_p.h"

// Buteo
#include <PluginCbInterface.h>
#include <LogMacros.h>
#include <ProfileEngineDefs.h>
#include <ProfileManager.h>

NextcloudBackupOperationClient::NextcloudBackupOperationClient(const QString& pluginName,
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

NextcloudBackupOperationClient::~NextcloudBackupOperationClient()
{
}

void NextcloudBackupOperationClient::connectivityStateChanged(Sync::ConnectivityType type, bool state)
{
    LOG_DEBUG("Received connectivity change event:" << type << " changed to " << state);
    if (type == Sync::CONNECTIVITY_INTERNET && !state) {
        // we lost connectivity during sync.
        abortSync(Sync::SYNC_CONNECTION_ERROR);
    }
}

bool NextcloudBackupOperationClient::init()
{
    QString accountIdString = iProfile.key(Buteo::KEY_ACCOUNT_ID);
    m_accountId = accountIdString.toInt();
    if (m_accountId == 0) {
        LOG_CRITICAL("Nextcloud Backup profile does not specify" << Buteo::KEY_ACCOUNT_ID);
        return false;
    }

    m_syncDirection = iProfile.syncDirection();
    m_conflictResPolicy = iProfile.conflictResolutionPolicy();

    if (!m_syncer) {
        m_syncer = newSyncer();
        connect(m_syncer, &Syncer::syncSucceeded,
                this, &NextcloudBackupOperationClient::syncSucceeded);
        connect(m_syncer, &Syncer::syncFailed,
                this, &NextcloudBackupOperationClient::syncFailed);
    }

    return true;
}

bool NextcloudBackupOperationClient::uninit()
{
    delete m_syncer;
    m_syncer = 0;
    return true;
}

bool NextcloudBackupOperationClient::startSync()
{
    if (m_accountId == 0) {
        return false;
    }

    m_syncer->startSync(m_accountId);
    return true;
}

void NextcloudBackupOperationClient::syncSucceeded()
{
    syncFinished(Buteo::SyncResults::NO_ERROR, QString());
}

void NextcloudBackupOperationClient::syncFailed()
{
    syncFinished(Buteo::SyncResults::INTERNAL_ERROR, QString());
}

void NextcloudBackupOperationClient::abortSync(Sync::SyncStatus status)
{
    abort(status);
}

void NextcloudBackupOperationClient::abort(Sync::SyncStatus status)
{
    m_syncer->abortSync();
    syncFinished(status, QStringLiteral("Sync aborted"));
}

void NextcloudBackupOperationClient::syncFinished(int minorErrorCode, const QString &message)
{
    if (minorErrorCode == Buteo::SyncResults::NO_ERROR) {
        LOG_DEBUG("Nextcloud Backup sync succeeded!" << message);
        m_results = Buteo::SyncResults(QDateTime::currentDateTimeUtc(),
                                       Buteo::SyncResults::SYNC_RESULT_SUCCESS,
                                       Buteo::SyncResults::NO_ERROR);
        emit success(getProfileName(), message);
    } else {
        LOG_CRITICAL("Nextcloud Backup sync failed:" << minorErrorCode << message);
        m_results = Buteo::SyncResults(iProfile.lastSuccessfulSyncTime(), // don't change the last sync time
                                       Buteo::SyncResults::SYNC_RESULT_FAILED,
                                       minorErrorCode);
        emit error(getProfileName(), message, minorErrorCode);
    }
}

Buteo::SyncResults NextcloudBackupOperationClient::getSyncResults() const
{
    return m_results;
}

bool NextcloudBackupOperationClient::cleanUp()
{
    // This function is called after the account has been deleted.
    QString accountIdString = iProfile.key(Buteo::KEY_ACCOUNT_ID);
    m_accountId = accountIdString.toInt();
    if (m_accountId == 0) {
        LOG_CRITICAL("Nextcloud Backup profile does not specify" << Buteo::KEY_ACCOUNT_ID);
        return false;
    }

    if (!m_syncer) {
        m_syncer = newSyncer();
    }

    m_syncer->purgeAccount(m_accountId);
    delete m_syncer;
    m_syncer = 0;

    return true;
}