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

#include "imagecache.h"

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QPair>
#include <QNetworkAccessManager>

class ReplyParser;

class Syncer : public WebDavSyncer
{
    Q_OBJECT

public:
    Syncer(QObject *parent, Buteo::SyncProfile *profile);
   ~Syncer();

    void purgeAccount(int accountId) Q_DECL_OVERRIDE;

private Q_SLOTS:
    void handleAlbumContentMetaDataReply();

private:
    void beginSync() Q_DECL_OVERRIDE;
    bool performAlbumContentMetadataRequest(const QString &serverUrl, const QString &albumPath, const QString &parentAlbumPath);
    void calculateAndApplyDelta();

    ReplyParser *m_replyParser = nullptr;
    QList<QNetworkReply*> m_requestQueue;
    QHash<QString, SyncCache::Album> m_albums;
    QHash<QString, SyncCache::Photo> m_photos;
};

#endif // NEXTCLOUD_IMAGES_SYNCER_P_H
