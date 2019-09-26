/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "xmlreplyparser_p.h"

#include <LogMacros.h>

#include <QList>
#include <QXmlStreamReader>

namespace {

QVariantMap elementToVariantMap(QXmlStreamReader &reader)
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
            QVariantMap subElement = elementToVariantMap(reader);
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

}

void XmlReplyParser::debugDumpData(const QString &data)
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

QVariantMap XmlReplyParser::xmlToVariantMap(QXmlStreamReader &reader)
{
    QVariantMap retn;
    while (!reader.atEnd() && !reader.hasError() && reader.readNextStartElement()) {
        QString elementName = reader.name().toString();
        QVariantMap element = elementToVariantMap(reader);
        retn.insert(elementName, element);
    }
    return retn;
}

QList<XmlReplyParser::Resource> XmlReplyParser::parsePropFindResponse(const QByteArray &propFindResponse, const QString &remotePath)
{
    /* We expect a response of the form:
        <?xml version="1.0" encoding="UTF-8"?>
        <d:multistatus xmlns:d="DAV:" xmlns:nc="http://nextcloud.org/ns" xmlns:oc="http://owncloud.org/ns" xmlns:s="http://sabredav.org/ns">
           <d:response>
              <d:href>/remote.php/dav/files/Username/</d:href>
              <d:propstat>
                 <d:prop>
                    <d:getlastmodified>Tue, 17 Sep 2019 06:39:33 GMT</d:getlastmodified>
                    <d:resourcetype>
                       <d:collection />
                    </d:resourcetype>
                    <d:quota-used-bytes>2505506</d:quota-used-bytes>
                    <d:quota-available-bytes>-3</d:quota-available-bytes>
                    <d:getetag>"5d807fa5bea23"</d:getetag>
                 </d:prop>
                 <d:status>HTTP/1.1 200 OK</d:status>
              </d:propstat>
           </d:response>
           <d:response>
              <d:href>/remote.php/dav/files/Username/Documents/</d:href>
              <d:propstat>
                 <d:prop>
                    <d:getlastmodified>Tue, 17 Sep 2019 06:39:33 GMT</d:getlastmodified>
                    <d:resourcetype>
                       <d:collection />
                    </d:resourcetype>
                    <d:quota-used-bytes>119824</d:quota-used-bytes>
                    <d:quota-available-bytes>-3</d:quota-available-bytes>
                    <d:getetag>"5d807fa5bea23"</d:getetag>
                 </d:prop>
                 <d:status>HTTP/1.1 200 OK</d:status>
              </d:propstat>
           </d:response>
           <d:response>
              <d:href>/remote.php/dav/files/Username/Storage-Share.png</d:href>
              <d:propstat>
                 <d:prop>
                    <d:getlastmodified>Tue, 17 Sep 2019 06:39:33 GMT</d:getlastmodified>
                    <d:getcontentlength>25671</d:getcontentlength>
                    <d:resourcetype />
                    <d:getetag>"2d391d9c53795329f51011d1af60158b"</d:getetag>
                    <d:getcontenttype>image/png</d:getcontenttype>
                 </d:prop>
                 <d:status>HTTP/1.1 200 OK</d:status>
              </d:propstat>
           </d:response>
        </d:multistatus>
    */

    debugDumpData(QString::fromUtf8(propFindResponse));

    QXmlStreamReader reader(propFindResponse);
    QVariantMap variantMap = XmlReplyParser::xmlToVariantMap(reader);

    QVariantMap multistatusMap = variantMap.value(QStringLiteral("multistatus")).toMap();
    QVariantList responses;

    QVariant response = multistatusMap.value(QStringLiteral("response"));
    if (response.type() == QVariant::List) {
        // multiple directory entries.
        responses = response.toList();
    } else {
        // only one directory entry.
        responses << response.toMap();
    }

    // parse the information about each entry (response element)
    QList<XmlReplyParser::Resource> result;
    for (const QVariant &rv : responses) {
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
        QDateTime lastModified;
        QString contentType;
        for (const QVariant &vpropstat : propstats) {
            const QVariantMap propstat = vpropstat.toMap();
            const QVariantMap &prop(propstat.value(QStringLiteral("prop")).toMap());
            if (prop.contains(QStringLiteral("resourcetype"))) {
                const QStringList resourceTypeKeys = prop.value(QStringLiteral("resourcetype")).toMap().keys();
                resourcetypeCollection = resourceTypeKeys.contains(QStringLiteral("collection"), Qt::CaseInsensitive);
            }
            if (prop.contains(QStringLiteral("getlastmodified"))) {
                lastModified = QDateTime::fromString(prop.value(QStringLiteral("getlastmodified")).toMap().value(QStringLiteral("@text")).toString(), Qt::RFC2822Date);
            }
            if (prop.contains(QStringLiteral("getcontenttype"))) {
                contentType = prop.value(QStringLiteral("getcontenttype")).toMap().value(QStringLiteral("@text")).toString();
            }
        }

        if (entryHref != remotePath) {
            Resource resource;
            resource.lastModified = lastModified;
            resource.href = entryHref;
            resource.contentType = contentType;
            resource.isCollection = resourcetypeCollection;
            result.append(resource);
        }
    }

    return result;
}
