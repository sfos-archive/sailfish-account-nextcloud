/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Nextcloud account package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#include "replyparser_p.h"
#include "syncer_p.h"

namespace {

QString parentAlbumId(const QString &albumId)
{
    const int baseNameSep = albumId.lastIndexOf('/', -2); // ignore any trailing slash
    if (baseNameSep < 0) {
        return QString();
    }
    return albumId.mid(0, baseNameSep + 1);
}

QString appendDirSeparator(const QString &path)
{
    return path.endsWith('/') ? path : path + '/';
}

QString removeEdgeDirSeparators(const QString &path)
{
    return path.endsWith('/') ? path.mid(0, path.length() - 1) : path;
}

}

ReplyParser::GalleryMetadata ReplyParser::galleryMetadataFromResources(Syncer *imageSyncer,
                                                                       const QString &rootPath,
                                                                       const QString &queriedAlbumPath,
                                                                       const QList<NetworkReplyParser::Resource> &resources)
{
    ReplyParser::GalleryMetadata metadata;
    int queriedAlbumPhotoCount = 0;
    const QString normalizedRootPath = appendDirSeparator(rootPath);
    const QString normalizedQueriedAlbumPath = appendDirSeparator(queriedAlbumPath);

    for (const NetworkReplyParser::Resource &resource : resources) {
        if (!resource.isCollection
                && !resource.contentType.startsWith(QStringLiteral("image/"), Qt::CaseInsensitive)) {
            continue; // TODO: support video also.
        }

        QString albumId;
        if (resource.isCollection) {
            albumId = appendDirSeparator(resource.href);
        } else {
            albumId = resource.href.mid(0, resource.href.lastIndexOf('/') + 1);
        }
        const QString albumName = removeEdgeDirSeparators(albumId.mid(normalizedRootPath.length()));  // relative path; empty if albumId is root directory
        const bool isQueriedAlbum = (resource.isCollection && normalizedQueriedAlbumPath == albumId);

        // Examples:
        // normalizedRootPath = /remote.php/dav/files/Me/Photos/
        // normalizedQueriedAlbumPath = /remote.php/dav/files/Me/Photos/mydir/ (if mydir is the queried album)
        // albumId = /remote.php/dav/files/Me/Photos/mydir/
        // albumName = Photos/mydir
        // resource.href = [file] /remote.php/dav/files/Me/Photos/mydir/20191011_143815.jpg
        //        [or collection] /remote.php/dav/files/Me/Photos/mydir/

        if (resource.isCollection) {
            SyncCache::Album newAlbum;
            SyncCache::Album *album = isQueriedAlbum ? &metadata.album : &newAlbum;

            album->accountId = imageSyncer->accountId();
            album->userId = resource.ownerId;
            album->albumId = albumId;
            album->albumName = albumName;
            if (isQueriedAlbum) {
                album->parentAlbumId = normalizedQueriedAlbumPath == normalizedRootPath
                        ? QString()
                        : parentAlbumId(albumId);
            } else {
                album->parentAlbumId = normalizedQueriedAlbumPath;
            }
            album->photoCount = 0;
            album->etag = resource.etag;

            if (!isQueriedAlbum) {
                metadata.subAlbums.append(newAlbum);
            }
        } else {
            SyncCache::Photo photo;
            photo.accountId = imageSyncer->accountId();
            photo.userId = resource.ownerId;
            photo.albumId = albumId;
            photo.photoId = resource.fileId;
            photo.updatedTimestamp = resource.lastModified;
            photo.createdTimestamp = photo.updatedTimestamp;
            photo.fileName = resource.href.mid(resource.href.lastIndexOf('/') + 1);
            photo.albumPath = albumName;
            photo.imageUrl = imageSyncer->serverUrl();
            photo.imageUrl.setPath(resource.href);
            photo.fileSize = resource.size;
            photo.fileType = resource.contentType;
            photo.etag = resource.etag;

            metadata.photos.append(photo);
            queriedAlbumPhotoCount++;
        }
    }

    metadata.album.photoCount = queriedAlbumPhotoCount;
    return metadata;
}
