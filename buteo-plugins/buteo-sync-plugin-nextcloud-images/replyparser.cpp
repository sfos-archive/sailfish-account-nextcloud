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

#include <LogMacros.h>

#include <QString>
#include <QList>
#include <QXmlStreamReader>
#include <QByteArray>
#include <QRegularExpression>

namespace {
    void debugDumpData(const QString &data)
    {
        if (Buteo::Logger::instance()->getLogLevel() < 7) {
            return;
        }

        QString dbgout;
        Q_FOREACH (const QChar &c, data) {
            if (c == '\r' || c == '\n') {
                if (!dbgout.isEmpty()) {
                    LOG_DEBUG(dbgout);
                    dbgout.clear();
                }
            } else {
                dbgout += c;
            }
        }
        if (!dbgout.isEmpty()) {
            LOG_DEBUG(dbgout);
        }
    }

    QVariantMap elementToVMap(QXmlStreamReader &reader)
    {
        QVariantMap element;

        // store the attributes of the element
        QXmlStreamAttributes attrs = reader.attributes();
        while (attrs.size()) {
            QXmlStreamAttribute attr = attrs.takeFirst();
            element.insert(attr.name().toString(), attr.value().toString());
        }

        while (reader.readNext() != QXmlStreamReader::EndElement) {
            if (reader.isCharacters()) {
                // store the text of the element, if any
                QString elementText = reader.text().toString();
                if (!elementText.isEmpty()) {
                    element.insert(QLatin1String("@text"), elementText);
                }
            } else if (reader.isStartElement()) {
                // recurse if necessary.
                QString subElementName = reader.name().toString();
                QVariantMap subElement = elementToVMap(reader);
                if (element.contains(subElementName)) {
                    // already have an element with this name.
                    // create a variantlist and append.
                    QVariant existing = element.value(subElementName);
                    QVariantList subElementList;
                    if (existing.type() == QVariant::Map) {
                        // we need to convert the value into a QVariantList
                        subElementList << existing.toMap();
                    } else if (existing.type() == QVariant::List) {
                        subElementList = existing.toList();
                    }
                    subElementList << subElement;
                    element.insert(subElementName, subElementList);
                } else {
                    // first element with this name.  insert as a map.
                    element.insert(subElementName, subElement);
                }
            }
        }

        return element;
    }

    QVariantMap xmlToVMap(QXmlStreamReader &reader)
    {
        QVariantMap retn;
        while (!reader.atEnd() && !reader.hasError() && reader.readNextStartElement()) {
            QString elementName = reader.name().toString();
            QVariantMap element = elementToVMap(reader);
            retn.insert(elementName, element);
        }
        return retn;
    }
}

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
    debugDumpData(QString::fromUtf8(albumContentMetadataResponse));
    QXmlStreamReader reader(albumContentMetadataResponse);
    QVariantMap vmap = xmlToVMap(reader);
    QVariantMap multistatusMap = vmap[QLatin1String("multistatus")].toMap();
    QVariantList responses;
    if (multistatusMap[QLatin1String("response")].type() == QVariant::List) {
        // multiple directory entries.
        responses = multistatusMap[QLatin1String("response")].toList();
    } else {
        // only one directory entry.
        QVariantMap response = multistatusMap[QLatin1String("response")].toMap();
        responses << response;
    }

    // parse the information about each entry (response element)
    ReplyParser::ContentMetadata contentMetadata;
    Q_FOREACH (const QVariant &rv, responses) {
        const QVariantMap rmap = rv.toMap();
        const QString entryHref = QString::fromUtf8(
                QByteArray::fromPercentEncoding(
                        rmap.value(QStringLiteral("href")).toMap().value(
                                QStringLiteral("@text")).toString().toUtf8()));
        QVariantList propstats;
        if (rmap.value(QStringLiteral("propstat")).type() == QVariant::List) {
            propstats = rmap.value(QStringLiteral("propstat")).toList();
        } else {
            QVariantMap propstat = rmap.value(QStringLiteral("propstat")).toMap();
            propstats << propstat;
        }
        bool resourcetypeCollection = false;
        QString lastModified;
        QString contentType;
        for (const QVariant &vpropstat : propstats) {
            const QVariantMap propstat = vpropstat.toMap();
            const QVariantMap &prop(propstat.value(QStringLiteral("prop")).toMap());
            if (prop.contains(QStringLiteral("resourcetype"))) {
                const QStringList resourceTypeKeys = prop.value(QStringLiteral("resourcetype")).toMap().keys();
                resourcetypeCollection = resourceTypeKeys.contains(QStringLiteral("collection"), Qt::CaseInsensitive);
            }
            if (prop.contains(QStringLiteral("getlastmodified"))) {
                lastModified = prop.value(QStringLiteral("getlastmodified")).toMap().value(QStringLiteral("@text")).toString();
            }
            if (prop.contains(QStringLiteral("getcontenttype"))) {
                contentType = prop.value(QStringLiteral("getcontenttype")).toMap().value(QStringLiteral("@text")).toString();
            }
        }

        if (resourcetypeCollection && entryHref != albumPath) {
            SyncCache::Album album;
            album.accountId = m_accountId;
            album.userId = m_userId;
            album.albumId = entryHref;
            album.parentAlbumId = albumPath;
            album.albumName = entryHref.mid(m_webdavPath.length());
            if (album.albumName.endsWith('/')) {
                album.albumName.chop(1);
            }
            album.photoCount = -1;
            contentMetadata.albums.append(album);
        } else if (!resourcetypeCollection && contentType.startsWith(QStringLiteral("image/"), Qt::CaseInsensitive)) {
            SyncCache::Photo photo;
            photo.accountId = m_accountId;
            photo.userId = m_userId;
            photo.albumId = albumPath;
            photo.photoId = entryHref;
            photo.fileName = QUrl(entryHref).fileName();
            photo.albumPath = albumPath.mid(m_webdavPath.length());
            if (photo.albumPath.endsWith('/')) {
                photo.albumPath.chop(1);
            }
            photo.createdTimestamp = lastModified;
            photo.updatedTimestamp = lastModified;
            photo.imageUrl = m_serverUrl;
            photo.imageUrl.setPath(entryHref);
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

