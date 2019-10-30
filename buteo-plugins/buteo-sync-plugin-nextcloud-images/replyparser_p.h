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

#include "imagecache.h"

class Syncer;
class ReplyParser
{
public:
    struct GalleryMetadata {
        QVector<SyncCache::Album> albums;
        QVector<SyncCache::Photo> photos;
    };

    ReplyParser(Syncer *parent, int accountId, const QString &userId, const QUrl &serverUrl, const QString &webdavPath);
    ~ReplyParser();

    GalleryMetadata parseGalleryMetadata(const QByteArray &galleryListResponse);

private:
    Syncer *q;
    int m_accountId;
    QString m_userId;
    QUrl m_serverUrl;
    QString m_webdavPath;
};

Q_DECLARE_METATYPE(ReplyParser::GalleryMetadata)

#endif // NEXTCLOUD_IMAGES_REPLYPARSER_P_H

