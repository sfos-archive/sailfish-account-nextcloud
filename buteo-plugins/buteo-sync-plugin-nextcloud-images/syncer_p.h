/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_IMAGES_SYNCER_P_H
#define NEXTCLOUD_IMAGES_SYNCER_P_H

#include "imagecache.h"

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QPair>
#include <QNetworkAccessManager>

class AccountAuthenticator;
class WebDavRequestGenerator;
class ReplyParser;
namespace Buteo { class SyncProfile; }

class Syncer : public QObject
{
    Q_OBJECT

public:
    Syncer(QObject *parent, Buteo::SyncProfile *profile);
   ~Syncer();

    void startSync(int accountId);
    void purgeAccount(int accountId);
    void abortSync();

Q_SIGNALS:
    void syncSucceeded();
    void syncFailed();

private Q_SLOTS:
    void sync(const QString &serverUrl, const QString &addressbookPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors);
    void handleAlbumContentMetaDataReply();
    void signInError();
    void webDavError(int httpCode = 0);

private:
    bool performAlbumContentMetadataRequest(const QString &serverUrl, const QString &albumPath, const QString &parentAlbumPath);
    void calculateAndApplyDelta();

    Buteo::SyncProfile *m_syncProfile = nullptr;
    AccountAuthenticator *m_auth = nullptr;
    WebDavRequestGenerator *m_requestGenerator = nullptr;
    ReplyParser *m_replyParser = nullptr;
    QNetworkAccessManager m_qnam;
    bool m_syncAborted = false;
    bool m_syncError = false;

    // auth related
    int m_accountId = 0;
    QString m_serverUrl;
    QString m_webdavPath;
    QString m_username;
    QString m_password;
    QString m_accessToken;
    bool m_ignoreSslErrors = false;

    // request queue
    QList<QNetworkReply*> m_requestQueue;
    QHash<QString, SyncCache::Album> m_albums;
    QHash<QString, SyncCache::Photo> m_photos;
};

#endif // NEXTCLOUD_IMAGES_SYNCER_P_H
