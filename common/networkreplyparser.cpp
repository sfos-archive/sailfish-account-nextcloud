/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "networkreplyparser_p.h"

#include <QDebug>
#include <QList>
#include <QXmlStreamReader>
#include <QJsonDocument>
#include <QJsonParseError>

const QString XmlReplyParser::XmlElementTextKey = QStringLiteral("@text");

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
            if (!elementText.trimmed().isEmpty()) {
                element.insert(XmlReplyParser::XmlElementTextKey, elementText);
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

int ocsMetaStatusCode(const QVariantMap &ocsVariantMap)
{
    return ocsVariantMap.value(QStringLiteral("meta")).toMap()
            .value(QStringLiteral("statuscode")).toInt();
}

}

bool NetworkReplyParser::debugEnabled = false;

void NetworkReplyParser::debugDumpData(const QString &data)
{
    if (!debugEnabled) {
        return;
    }

    QString dbgout;
    Q_FOREACH (const QChar &c, data) {
        if (c == '\r' || c == '\n') {
            if (!dbgout.isEmpty()) {
                qDebug() << dbgout;
                dbgout.clear();
            }
        } else {
            dbgout += c;
        }
    }
    if (!dbgout.isEmpty()) {
        qDebug() << dbgout;
    }
}

//--- JsonReplyParser:

QVariantMap JsonReplyParser::findCapability(const QString &capabilityName, const QByteArray &capabilityResponse)
{
    /* We expect a response of the form:

    {
      "ocs": {
        ...
        "data": {
          ...
          "capabilities": {
            ...
            "notifications": {
              "ocs-endpoints": [
                "list",
                "get",
                "delete",
                "delete-all",
                "icons",
                "rich-strings",
                "action-web"
              ]
            }
          }
        }
      }
    }
    */

    NetworkReplyParser::debugDumpData(QString::fromUtf8(capabilityResponse));

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(capabilityResponse, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "JSON parsing failed:" << err.errorString();
        return QVariantMap();
    }

    const QVariant variant = doc.toVariant();
    if (variant.type() != QVariant::Map) {
        qWarning() << "Cannot convert JSON to map!";
        return QVariantMap();
    }

    const QVariantMap ocs = variant.toMap().value(QStringLiteral("ocs")).toMap();
    if (ocsMetaStatusCode(ocs) != 200) {
        qWarning() << "Capabilities response has non-success status code:" << ocsMetaStatusCode(ocs);
        return QVariantMap();
    }

    const QVariantMap capabilityMap = ocs.value(QStringLiteral("data")).toMap().value(QStringLiteral("capabilities")).toMap();
    if (capabilityMap.isEmpty()) {
        qWarning() << "No capabilities found in capability response!";
        return QVariantMap();
    }

    return capabilityMap.value(capabilityName).toMap();
}

QList<NetworkReplyParser::Notification> JsonReplyParser::parseNotificationResponse(const QByteArray &replyData)
{
    /* Expected result:
    {
      "ocs": {
        "meta": {
          "status": "ok",
          "statuscode": 200,
          "message": null
        },
        "data": [
          {
            "notification_id": 61,
            "app": "files_sharing",
            "user": "admin",
            "datetime": "2004-02-12T15:19:21+00:00",
            "object_type": "remote_share",
            "object_id": "13",
            "subject": "You received admin@localhost as a remote share from test",
            "subjectRich": "You received {share} as a remote share from {user}",
            "subjectRichParameters": {
              "share": {
                "type": "pending-federated-share",
                "id": "1",
                "name": "test"
              },
              "user": {
                "type": "user",
                "id": "test1",
                "name": "User One",
                "server": "http:\/\/nextcloud11.local"
              }
            },
            "message": "",
            "messageRich": "",
            "messageRichParameters": [],
            "link": "http://localhost/index.php/apps/files_sharing/pending",
            "icon": "http://localhost/img/icon.svg",
            "actions": [
              {
                "label": "Accept",
                "link": "http:\/\/localhost\/ocs\/v1.php\/apps\/files_sharing\/api\/v1\/remote_shares\/13",
                "type": "POST",
                "primary": true
              },
              {
                "label": "Decline",
                "link": "http:\/\/localhost\/ocs\/v1.php\/apps\/files_sharing\/api\/v1\/remote_shares\/13",
                "type": "DELETE",
                "primary": false
              }
            ]
          }
        ]
      }
    }
    */

    NetworkReplyParser::debugDumpData(QString::fromUtf8(replyData));

    QList<NetworkReplyParser::Notification> notifs;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(replyData, &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "JSON parsing failed:" << err.errorString();
        return notifs;
    }

    const QVariant variant = doc.toVariant();
    if (variant.type() != QVariant::Map) {
        qWarning() << "Cannot convert JSON to map!";
        return notifs;
    }

    const QVariantMap ocs = variant.toMap().value(QStringLiteral("ocs")).toMap();
    if (ocsMetaStatusCode(ocs) != 200) {
        qWarning() << "Notifications response has non-success status code:" << ocsMetaStatusCode(ocs);
        return notifs;
    }

    const QVariantList notificationList = ocs.value(QStringLiteral("data")).toList();
    for (const QVariant &notifVariant : notificationList) {
        const QVariantMap notifMap = notifVariant.toMap();

        NetworkReplyParser::Notification notif;
        notif.notificationId = notifMap.value(QStringLiteral("notification_id")).toString();
        notif.app = notifMap.value(QStringLiteral("app")).toString();
        notif.userName = notifMap.value(QStringLiteral("user")).toString();
        notif.dateTime = QDateTime::fromString(notifMap.value(QStringLiteral("datetime")).toString(), Qt::ISODate);
        notif.icon = notifMap.value(QStringLiteral("icon")).toString();
        notif.link = notifMap.value(QStringLiteral("link")).toString();
        notif.actions = notifMap.value(QStringLiteral("actions")).toList();

        notif.objectType = notifMap.value(QStringLiteral("object_type")).toString();
        notif.objectId = notifMap.value(QStringLiteral("object_id")).toString();

        notif.subject = notifMap.value(QStringLiteral("subject")).toString();
        notif.subjectRich = notifMap.value(QStringLiteral("subjectRich")).toString();
        notif.subjectRichParameters = notifMap.value(QStringLiteral("subjectRichParameters"));

        notif.message = notifMap.value(QStringLiteral("message")).toString();
        notif.messageRich = notifMap.value(QStringLiteral("messageRich")).toString();
        notif.messageRichParameters = notifMap.value(QStringLiteral("messageRichParameters"));

        notifs.append(notif);
    }

    return notifs;
}

//--- XmlReplyParser:

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

QList<NetworkReplyParser::Resource> XmlReplyParser::parsePropFindResponse(const QByteArray &propFindResponse, const QString &remotePath)
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

    NetworkReplyParser::debugDumpData(QString::fromUtf8(propFindResponse));

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
    QList<NetworkReplyParser::Resource> result;
    for (const QVariant &rv : responses) {
        const QVariantMap rmap = rv.toMap();
        const QString entryHref = QString::fromUtf8(
                QByteArray::fromPercentEncoding(
                        rmap.value(QStringLiteral("href")).toMap().value(
                                XmlElementTextKey).toString().toUtf8()));
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
                lastModified = QDateTime::fromString(prop.value(QStringLiteral("getlastmodified")).toMap().value(XmlElementTextKey).toString(), Qt::RFC2822Date);
            }
            if (prop.contains(QStringLiteral("getcontenttype"))) {
                contentType = prop.value(QStringLiteral("getcontenttype")).toMap().value(XmlElementTextKey).toString();
            }
        }

        if (entryHref != remotePath) {
            NetworkReplyParser::Resource resource;
            resource.lastModified = lastModified;
            resource.href = entryHref;
            resource.contentType = contentType;
            resource.isCollection = resourcetypeCollection;
            result.append(resource);
        }
    }

    return result;
}
