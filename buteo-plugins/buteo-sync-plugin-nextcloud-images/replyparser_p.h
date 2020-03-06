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
#include <QString>
#include <QList>
#include <QByteArray>

#include "synccacheimages.h"

class Syncer;
class ReplyParser
{
public:
    struct GalleryMetadata {
        QVector<SyncCache::Album> albums;
        QVector<SyncCache::Photo> photos;
        QString currAlbumId;
    };

    static GalleryMetadata parseGalleryMetadata(Syncer *imageSyncer, const QByteArray &galleryListResponse);
};

Q_DECLARE_METATYPE(ReplyParser::GalleryMetadata)

#endif // NEXTCLOUD_IMAGES_REPLYPARSER_P_H

