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

ReplyParser::ReplyParser(Syncer *parent, int accountId, const QString &userId, const QUrl &serverUrl, const QString &webdavPath)
    : q(parent), m_accountId(accountId), m_userId(userId), m_serverUrl(serverUrl), m_webdavPath(webdavPath)
{
}

ReplyParser::~ReplyParser()
{
}

ReplyParser::ContentMetadata ReplyParser::parseAlbumContentMetadata(
        const QByteArray &albumContentMetadataResponse,
        const QString &albumPath,
        const QString &parentAlbumPath)
{
    /* We expect a response of the form:
        <?xml version="1.0"?>
        <d:multistatus xmlns:d="DAV:" xmlns:s="http://sabredav.org/ns" xmlns:oc="http://owncloud.org/ns" xmlns:nc="http://nextcloud.org/ns">
            <d:response>
                <d:href>/remote.php/webdav/Photos/AlbumOne/</d:href>
                <d:propstat>
                    <d:prop>
                        <d:getlastmodified>Fri, 13 Sep 2019 05:50:43 GMT</d:getlastmodified>
                        <d:resourcetype><d:collection/></d:resourcetype>
                        <d:quota-used-bytes>2842338</d:quota-used-bytes>
                        <d:quota-available-bytes>-3</d:quota-available-bytes>
                        <d:getetag>&quot;5d7b2e33211ec&quot;</d:getetag>
                    </d:prop>
                    <d:status>HTTP/1.1 200 OK</d:status>
                </d:propstat>
            </d:response>
            <d:response>
                <d:href>/remote.php/webdav/Photos/AlbumOne/narawntapu_lake.jpg</d:href>
                <d:propstat>
                    <d:prop>
                        <d:getlastmodified>Fri, 13 Sep 2019 05:50:43 GMT</d:getlastmodified>
                        <d:getcontentlength>2842338</d:getcontentlength>
                        <d:resourcetype/>
                        <d:getetag>&quot;c3acea454fab91e5014ea78fd5d58246&quot;</d:getetag>
                        <d:getcontenttype>image/jpeg</d:getcontenttype>
                    </d:prop>
                    <d:status>HTTP/1.1 200 OK</d:status>
                </d:propstat>
                <d:propstat>
                    <d:prop>
                        <d:quota-used-bytes/>
                        <d:quota-available-bytes/>
                    </d:prop>
                    <d:status>HTTP/1.1 404 Not Found</d:status>
                </d:propstat>
            </d:response>
        </d:multistatus>
    */
    const QList<NetworkReplyParser::Resource> resourceList = XmlReplyParser::parsePropFindResponse(
            albumContentMetadataResponse, albumPath);
    ReplyParser::ContentMetadata contentMetadata;

    for (const NetworkReplyParser::Resource &resource : resourceList) {
        if (resource.isCollection && resource.href != albumPath) {
            SyncCache::Album album;
            album.accountId = m_accountId;
            album.userId = m_userId;
            album.albumId = resource.href;
            album.parentAlbumId = albumPath;
            album.albumName = resource.href.mid(m_webdavPath.length());
            if (album.albumName.endsWith('/')) {
                album.albumName.chop(1);
            }
            album.photoCount = -1;
            contentMetadata.albums.append(album);
        } else if (!resource.isCollection && resource.contentType.startsWith(QStringLiteral("image/"), Qt::CaseInsensitive)) {
            SyncCache::Photo photo;
            photo.accountId = m_accountId;
            photo.userId = m_userId;
            photo.albumId = albumPath;
            photo.photoId = resource.href;
            photo.fileName = QUrl(resource.href).fileName();
            photo.albumPath = albumPath.mid(m_webdavPath.length());
            if (photo.albumPath.endsWith('/')) {
                photo.albumPath.chop(1);
            }
            photo.createdTimestamp = resource.lastModified.toString(Qt::RFC2822Date);
            photo.updatedTimestamp = photo.createdTimestamp;
            photo.imageUrl = m_serverUrl;
            photo.imageUrl.setPath(resource.href);
            contentMetadata.photos.append(photo);
        }
    }

    // update information about this album.
    contentMetadata.album.accountId = m_accountId;
    contentMetadata.album.userId = m_userId;
    contentMetadata.album.albumId = albumPath;
    contentMetadata.album.parentAlbumId = parentAlbumPath;
    contentMetadata.album.albumName = albumPath.mid(m_webdavPath.length());
    if (contentMetadata.album.albumName.endsWith('/')) {
        contentMetadata.album.albumName.chop(1);
    }
    contentMetadata.album.photoCount = contentMetadata.photos.count();

    return contentMetadata;
}

