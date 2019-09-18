/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_BACKUP_REQUESTGENERATOR_P_H
#define NEXTCLOUD_BACKUP_REQUESTGENERATOR_P_H

#include <QString>
#include <QNetworkReply>
#include <QNetworkAccessManager>

class Syncer;
class RequestGenerator
{
public:
    RequestGenerator(Syncer *parent, const QString &username, const QString &password);
    RequestGenerator(Syncer *parent, const QString &accessToken);

    QNetworkReply *dirListing(const QString &serverUrl, const QString &remoteDirPath);
    QNetworkReply *dirCreation(const QString &serverUrl, const QString &remoteDirPath);
    QNetworkReply *upload(const QString &serverUrl, const QString &dataContentType, const QByteArray &data, const QString &remoteDirPath);
    QNetworkReply *download(const QString &serverUrl, const QString &remoteFilePath);

private:
    QNetworkReply *generateRequest(const QString &url,
                                   const QString &path,
                                   const QString &depth,
                                   const QString &requestType,
                                   const QString &request) const;
    QNetworkReply *generateDataRequest(const QString &url,
                                       const QString &path,
                                       const QString &requestType,
                                       const QByteArray &requestData,
                                       const QString &requestDataContentType) const;
    Syncer *q;
    QString m_username;
    QString m_password;
    QString m_accessToken;
};

#endif // NEXTCLOUD_BACKUP_REQUESTGENERATOR_P_H
