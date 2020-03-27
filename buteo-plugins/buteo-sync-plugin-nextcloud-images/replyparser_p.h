/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_IMAGES_REPLYPARSER_P_H
#define NEXTCLOUD_IMAGES_REPLYPARSER_P_H

#include <QObject>
#include <QList>

#include "synccacheimages.h"
#include "networkreplyparser_p.h"

class Syncer;
class ReplyParser
{
public:
    struct GalleryMetadata {
        QVector<SyncCache::Album> albums;
        QVector<SyncCache::Photo> photos;
        QString currAlbumId;
    };

    static GalleryMetadata galleryMetadataFromResources(Syncer *imageSyncer,
                                                        const QString &rootPath,
                                                        const QString &albumRemotePath,
                                                        const QList<NetworkReplyParser::Resource> &resources);
};

Q_DECLARE_METATYPE(ReplyParser::GalleryMetadata)

#endif // NEXTCLOUD_IMAGES_REPLYPARSER_P_H

