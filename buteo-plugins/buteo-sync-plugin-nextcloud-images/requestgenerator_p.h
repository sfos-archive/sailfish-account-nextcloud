/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_IMAGES_REQUESTGENERATOR_P_H
#define NEXTCLOUD_IMAGES_REQUESTGENERATOR_P_H

#include <QString>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class Syncer;
class RequestGenerator
{
public:
    RequestGenerator(Syncer *parent, const QString &username, const QString &password);
    RequestGenerator(Syncer *parent, const QString &accessToken);

    QNetworkReply *albumContentMetadata(const QString &serverUrl, const QString &albumPath);

private:
    QNetworkReply *generateRequest(const QString &url,
                                   const QString &path,
                                   const QString &depth,
                                   const QString &requestType,
                                   const QString &request) const;
    Syncer *q;
    QString m_username;
    QString m_password;
    QString m_accessToken;
};

#endif // NEXTCLOUD_IMAGES_REQUESTGENERATOR_P_H
