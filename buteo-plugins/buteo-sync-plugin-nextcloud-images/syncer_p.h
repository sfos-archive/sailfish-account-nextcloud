/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_IMAGES_SYNCER_P_H
#define NEXTCLOUD_IMAGES_SYNCER_P_H

#include "webdavsyncer_p.h"

#include "synccacheimages.h"

#include <QList>
#include <QHash>

class ReplyParser;
class JsonRequestGenerator;

class Syncer : public WebDavSyncer
{
    Q_OBJECT

public:
    Syncer(QObject *parent, Buteo::SyncProfile *profile);
   ~Syncer();

    void purgeAccount(int accountId) override;

private Q_SLOTS:
    void handleConfigReply();
    void handleGalleryMetaDataReply();

private:
    void beginSync() override;
    void calculateAndApplyDelta(
            const QHash<QString, SyncCache::Album> &albums,
            const QHash<QString, SyncCache::Photo> &photos);

    ReplyParser *m_replyParser = nullptr;
};

#endif // NEXTCLOUD_IMAGES_SYNCER_P_H
