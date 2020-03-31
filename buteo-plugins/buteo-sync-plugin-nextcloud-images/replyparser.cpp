/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "replyparser_p.h"
#include "syncer_p.h"

ReplyParser::GalleryMetadata ReplyParser::galleryMetadataFromResources(Syncer *imageSyncer,
                                                                       const QString &rootPath,
                                                                       const QString &albumRemotePath,
                                                                       const QList<NetworkReplyParser::Resource> &resources)
{
    ReplyParser::GalleryMetadata metadata;

    QHash<QString, int> albumPhotoCount;

    for (const NetworkReplyParser::Resource &resource : resources) {
        if (resource.isCollection
                || !resource.contentType.startsWith(QStringLiteral("image/"), Qt::CaseInsensitive)) {
            continue; // TODO: support video also.
        }

        const QString &photoPath = resource.href;
        const QString fullAlbumPath = albumRemotePath.endsWith('/')
                ? albumRemotePath.mid(0, albumRemotePath.length() - 1)
                : albumRemotePath;
        const QString relativeAlbumPath = fullAlbumPath.mid(rootPath.length());

        // Examples:
        // rootPath = /remote.php/dav/files/Me/Photos/
        // photoPath = /remote.php/dav/files/Me/Photos/mydir/20191011_143815.jpg
        // albumId = /remote.php/dav/files/Me/Photos/mydir
        // albumPath = Photos/mydir (the relative path; this is same as Album::albumName)

        SyncCache::Photo photo;
        photo.accountId = imageSyncer->accountId();
        photo.userId = resource.ownerId;
        photo.albumId = fullAlbumPath;
        photo.photoId = resource.fileId;
        photo.updatedTimestamp = resource.lastModified;
        photo.createdTimestamp = photo.updatedTimestamp;
        photo.fileName = photoPath.mid(photoPath.lastIndexOf('/') + 1);
        photo.albumPath = relativeAlbumPath;
        photo.imageUrl = imageSyncer->serverUrl();
        photo.imageUrl.setPath(photoPath);
        photo.fileSize = resource.size;
        photo.fileType = resource.contentType;
        metadata.photos.append(photo);

        int albumIndex = -1;
        for (int i = 0; i < metadata.albums.count(); ++i) {
            if (metadata.albums[i].albumId == photo.albumId) {
                albumIndex = i;
                break;
            }
        }

        if (albumIndex >= 0) {
            metadata.albums[albumIndex].photoCount++;
        } else {
            SyncCache::Album album;
            album.accountId = imageSyncer->accountId();
            album.userId = photo.userId;
            album.albumId = photo.albumId;
            album.albumName = photo.albumPath;  // may be empty if home directory
            album.parentAlbumId = albumRemotePath == rootPath
                    ? QString()
                    : album.albumId.mid(0, album.albumId.lastIndexOf('/'));
            album.photoCount = 1;
            metadata.albums.append(album);
        }
    }

    return metadata;
}
