/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_XMLREPLYPARSER_P_H
#define NEXTCLOUD_XMLREPLYPARSER_P_H

#include <QVariantMap>
#include <QDateTime>
#include <QUrl>

class QXmlStreamReader;

// TODO rename to NextcloudReplyParser or such since this parses both XML and JSON.
class XmlReplyParser
{
public:
    class Resource
    {
    public:
        QDateTime lastModified;
        QString href;
        QString contentType;
        bool isCollection = false;
    };

    class Notification
    {
    public:
        QString notificationId;
        QString app;
        QString userName;
        QDateTime dateTime;
        QString icon;
        QString link;
        QVariantList actions;

        QString objectType;
        QString objectId;

        QString subject;
        QString subjectRich;
        QVariant subjectRichParameters;

        QString message;
        QString messageRich;
        QVariant messageRichParameters;
    };

    static QVariantMap xmlToVariantMap(QXmlStreamReader &reader);

    static QList<Resource> parsePropFindResponse(const QByteArray &propFindResponse, const QString &remotePath);
    static QVariantMap findCapabilityfromJson(const QString &capabilityName, const QByteArray &capabilityResponse);
    static QList<Notification> parseNotificationsFromJson(const QByteArray &replyData);

    static void debugDumpData(const QString &data);

    static const QString XmlElementTextKey;
};

#endif // NEXTCLOUD_XMLREPLYPARSER_P_H


