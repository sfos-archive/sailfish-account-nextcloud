/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Copyright (C) 2021 Jolla Ltd.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_POSTS_CLIENT_H
#define NEXTCLOUD_POSTS_CLIENT_H

// Buteo
#include <ClientPlugin.h>
#include <SyncPluginLoader.h>
#include <SyncProfile.h>
#include <SyncResults.h>
#include <SyncCommonDefs.h>

#include <QtCore/QString>
#include <QtCore/QObject>

class Syncer;
class Q_DECL_EXPORT NextcloudPostsClient : public Buteo::ClientPlugin
{
    Q_OBJECT

public:
    NextcloudPostsClient(
            const QString &pluginName,
            const Buteo::SyncProfile &profile,
            Buteo::PluginCbInterface *cbInterface);
    ~NextcloudPostsClient();

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

    Syncer *m_syncer;
    Sync::SyncStatus m_syncStatus;
    Buteo::SyncResults m_results;
    Buteo::SyncProfile::SyncDirection m_syncDirection;
    Buteo::SyncProfile::ConflictResolutionPolicy m_conflictResPolicy;
    int m_accountId;
};

class NextcloudPostsClientLoader : public Buteo::SyncPluginLoader
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.sailfishos.plugins.sync.NextcloudPostsClientLoader")
    Q_INTERFACES(Buteo::SyncPluginLoader)

public:
    Buteo::ClientPlugin* createClientPlugin(const QString& pluginName,
                                            const Buteo::SyncProfile& profile,
                                            Buteo::PluginCbInterface* cbInterface) override;
};

#endif // NEXTCLOUD_POSTS_CLIENT_H
