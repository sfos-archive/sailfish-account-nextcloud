/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_IMAGES_CLIENT_H
#define NEXTCLOUD_IMAGES_CLIENT_H

// Buteo
#include <ClientPlugin.h>
#include <SyncProfile.h>
#include <SyncResults.h>
#include <SyncCommonDefs.h>

#include <QtCore/QString>
#include <QtCore/QObject>

class Syncer;
class Q_DECL_EXPORT NextcloudImagesClient : public Buteo::ClientPlugin
{
    Q_OBJECT

public:
    NextcloudImagesClient(
            const QString &pluginName,
            const Buteo::SyncProfile &profile,
            Buteo::PluginCbInterface *cbInterface);
    ~NextcloudImagesClient();

    bool init();
    bool uninit();
    bool startSync();
    void abortSync(Sync::SyncStatus aStatus = Sync::SYNC_ABORTED);
    Buteo::SyncResults getSyncResults() const;
    bool cleanUp();

public Q_SLOTS:
    void connectivityStateChanged(Sync::ConnectivityType aType, bool aState);

private Q_SLOTS:
    void syncSucceeded();
    void syncFailed();

private:
    void abort(Sync::SyncStatus status = Sync::SYNC_ABORTED);
    void syncFinished(int minorErrorCode, const QString &message);
    Buteo::SyncProfile::SyncDirection syncDirection();
    Buteo::SyncProfile::ConflictResolutionPolicy conflictResolutionPolicy();

    Syncer* m_syncer;
    Sync::SyncStatus m_syncStatus;
    Buteo::SyncResults m_results;
    Buteo::SyncProfile::SyncDirection m_syncDirection;
    Buteo::SyncProfile::ConflictResolutionPolicy m_conflictResPolicy;
    int m_accountId;
};

extern "C" NextcloudImagesClient* createPlugin(const QString &pluginName,
                                               const Buteo::SyncProfile &profile,
                                               Buteo::PluginCbInterface *cbInterface);

extern "C" void destroyPlugin(NextcloudImagesClient *client);

#endif // NEXTCLOUD_IMAGES_CLIENT_H
