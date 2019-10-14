/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_POSTS_SYNCER_P_H
#define NEXTCLOUD_POSTS_SYNCER_P_H

#include "webdavsyncer_p.h"

#include "eventcache.h"

#include <QObject>
#include <QString>

class ReplyParser;

class Syncer : public WebDavSyncer
{
    Q_OBJECT

public:
    Syncer(QObject *parent, Buteo::SyncProfile *profile);
   ~Syncer();

    void purgeAccount(int accountId) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void handleCapabilitiesReply();
    void handleNotificationListReply();

private:
    void beginSync() Q_DECL_OVERRIDE;
    bool performCapabilitiesRequest();
    bool performNotificationListRequest();
};

#endif // NEXTCLOUD_POSTS_SYNCER_P_H
