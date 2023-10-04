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

#ifndef NEXTCLOUD_NETWORKREPLYPARSER_P_H
#define NEXTCLOUD_NETWORKREPLYPARSER_P_H

#include <QVariantMap>
#include <QDateTime>
#include <QUrl>

class QXmlStreamReader;

class NetworkReplyParser
{
public:
    class User
    {
    public:
        QString userId;
        QString displayName;
    };

    class Resource
    {
    public:
        QDateTime lastModified;
        QString href;
        QString contentType;
        QString ownerId;
        QString fileId;
        QString etag;
        int size = 0;
        bool isCollection = false;
    };

    class Notification
    {
    public:
        QString notificationId;
        QString app;
        QString userId;
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

    static void debugDumpData(const QString &data);

    static bool debugEnabled;
};

class XmlReplyParser
{
public:
    static QVariantMap xmlToVariantMap(QXmlStreamReader &reader);
    static QList<NetworkReplyParser::Resource> parsePropFindResponse(const QByteArray &propFindResponse);

    static const QString XmlElementTextKey;
};

class JsonReplyParser
{
public:
    static QVariantMap findCapability(const QString &capabilityName, const QByteArray &capabilityResponse);
    static NetworkReplyParser::User parseUserResponse(const QByteArray &userInfoResponse);
    static QList<NetworkReplyParser::Notification> parseNotificationResponse(const QByteArray &replyData);
};

#endif // NEXTCLOUD_NETWORKREPLYPARSER_P_H


