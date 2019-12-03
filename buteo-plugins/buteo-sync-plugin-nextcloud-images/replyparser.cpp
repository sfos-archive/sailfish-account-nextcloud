/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "replyparser_p.h"
#include "networkreplyparser_p.h"
#include "syncer_p.h"

#include <QList>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

ReplyParser::GalleryMetadata ReplyParser::parseGalleryMetadata(Syncer *imageSyncer,
        const QByteArray &galleryListResponse)
{
    /* We expect a response of the form:
        {
            "files":[
                {"path":"Photos\/All_green.jpg","nodeid":1918,"mtime":1570162137,"etag":"21d6119483a039b18c0afac81b888be7","size":866192,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Archipelago_of_Turku.jpg","nodeid":1855,"mtime":1570002318,"etag":"b0792cbbf5c7954e2694f450fcd3b68a","size":1028936,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Coast.jpg","nodeid":711,"mtime":1568702360,"etag":"577dcaa281de0557d3a31fb777fe95d2","size":819766,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Connect.jpg","nodeid":1885,"mtime":1570081846,"etag":"464300d546286c81cb90a1417afc66cb","size":255068,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Crisp.jpg","nodeid":1915,"mtime":1570162117,"etag":"beebb619d1510f84811348d83e080087","size":523564,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Hummingbird.jpg","nodeid":714,"mtime":1568702360,"etag":"ca3c4eb07cd717c8abc5eaea27f1e931","size":585219,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Images.jpg","nodeid":1909,"mtime":1570082956,"etag":"cb334e5822961207dd53b2dd2516521c","size":524697,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Nut.jpg","nodeid":717,"mtime":1568702360,"etag":"5ef8f1f4fc963b45ad69b1e842ce2b0a","size":955026,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Outlook.jpg","nodeid":1816,"mtime":1570002083,"etag":"e17094096f80478f808a6e8e5502bf64","size":614230,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Serenity.jpg","nodeid":1858,"mtime":1570003412,"etag":"7dd16623afce289049e4b7a04a0c40cf","size":452235,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Together.jpg","nodeid":1810,"mtime":1570000466,"etag":"b81c574aaa7c427aa424b2f64ab8516d","size":1554170,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/Touch_of_Colours.jpg","nodeid":1912,"mtime":1570083130,"etag":"ffa0216da2c064bbfb107dde102fbbeb","size":1022674,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"},
                {"path":"Photos\/SubAlbum\/Curiosity.jpg","nodeid":1813,"mtime":1570000819,"etag":"643b07540d471de90520d7566ed89e39","size":1161357,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":27,"mimetype":"image\/jpeg"}
            ],
            "albums":{
                "Photos":{"path":"Photos","nodeid":708,"mtime":1571972922,"etag":"5db2673ad51ce","size":10363134,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":31,"freespace":109588643840},
                "Photos\/SubAlbum":{"path":"Photos\/SubAlbum","nodeid":2644,"mtime":1571972922,"etag":"5db2673ad51ce","size":1161357,"sharedwithuser":false,"owner":{"uid":"Chris","displayname":"Chris"},"permissions":31,"freespace":109588643840}
            },
            "albumconfig":[],
            "albumpath":"Photos",
            "updated":true
        }
    */
    ReplyParser::GalleryMetadata retn;

    QJsonParseError err;
    NetworkReplyParser::debugDumpData(QString::fromUtf8(galleryListResponse));
    const QJsonDocument doc = QJsonDocument::fromJson(galleryListResponse, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse gallery metadata response JSON:" << err.errorString();
        return retn;
    }

    const QJsonObject obj = doc.object();
    const QJsonArray files = obj.value("files").toArray();
    const QJsonObject albums = obj.value("albums").toObject();

    QHash<QString, int> albumPhotoCount;
    QHash<QString, QPair<QUrl,QString> > albumThumbnails;

    for (QJsonArray::const_iterator it = files.constBegin(); it != files.constEnd(); ++it) {
        const QJsonObject photoData = (*it).toObject();
        const QString mimeType = photoData.value("mimetype").toString();
        if (!mimeType.startsWith(QStringLiteral("image/"), Qt::CaseInsensitive)) {
            continue; // TODO: support video also.
        }
        const QString photoPath = photoData.value("path").toString();
        const int lastDirSep = photoPath.lastIndexOf('/');
        const QString albumPath = lastDirSep < 0 ? QString() : photoPath.mid(0, lastDirSep); // may be empty if home directory
        const int photoId = photoData.value("nodeid").toInt();

        SyncCache::Photo photo;
        photo.accountId = imageSyncer->accountId();
        photo.userId = photoData.value("owner").toObject().value("uid").toString();
        photo.albumId = QStringLiteral("%1%2").arg(imageSyncer->webDavPath(), albumPath);
        photo.photoId = QString::number(photoId);
        photo.updatedTimestamp = QDateTime::fromTime_t(photoData.value("mtime").toVariant().toUInt());
        photo.createdTimestamp = photo.updatedTimestamp;
        photo.fileName = photoPath.mid(photoPath.lastIndexOf('/') + 1);
        photo.albumPath = albumPath;
        photo.thumbnailUrl = imageSyncer->serverUrl();
        photo.thumbnailUrl.setPath(QStringLiteral("/index.php/apps/gallery/api/preview/%1/128/128").arg(photoId));
        photo.imageUrl = imageSyncer->serverUrl();
        photo.imageUrl.setPath(imageSyncer->webDavPath() + photoPath);
        photo.fileSize = photoData.value("size").toInt();
        photo.fileType = mimeType;
        retn.photos.append(photo);
        albumPhotoCount[photo.albumId]++;

        if (!albumThumbnails.contains(photo.albumId)) {
            albumThumbnails.insert(photo.albumId, qMakePair(photo.thumbnailUrl, photo.fileName));
        }
    }

    for (QJsonObject::const_iterator it = albums.constBegin(); it != albums.constEnd(); ++it) {
        const QJsonObject albumData = (*it).toObject();
        const QString albumName = it.key(); // may be empty if home directory
        SyncCache::Album album;
        album.albumId = QStringLiteral("%1%2").arg(imageSyncer->webDavPath(), albumName);
        album.photoCount = albumPhotoCount.value(album.albumId);
        if (album.photoCount == 0) {
            continue;   // ignore empty albums
        }

        album.accountId = imageSyncer->accountId();
        album.userId = albumData.value("owner").toObject().value("uid").toString();
        album.parentAlbumId = albumName.contains('/')
                ? QStringLiteral("%1%2").arg(imageSyncer->webDavPath(), albumName.mid(0, albumName.lastIndexOf('/')))
                : imageSyncer->webDavPath();
        album.albumName = albumName;

        const QPair<QUrl,QString> &thumbnail = albumThumbnails.value(album.albumId);
        album.thumbnailUrl = thumbnail.first;
        album.thumbnailFileName = thumbnail.second;

        retn.albums.append(album);
    }

    return retn;
}
