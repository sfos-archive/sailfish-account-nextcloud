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

class QXmlStreamReader;

class XmlReplyParser
{
public:
    class Resource {
    public:
        QDateTime lastModified;
        QString href;
        QString contentType;
        bool isCollection = false;
    };

    static QVariantMap xmlToVariantMap(QXmlStreamReader &reader);

    static QList<Resource> parsePropFindResponse(const QByteArray &propFindResponse, const QString &remotePath);

    static void debugDumpData(const QString &data);
};

#endif // NEXTCLOUD_XMLREPLYPARSER_P_H


