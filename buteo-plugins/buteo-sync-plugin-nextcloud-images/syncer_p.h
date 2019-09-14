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

class tst_replyparser;
class Auth;
class RequestGenerator;
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
    friend class RequestGenerator;
    friend class tst_replyparser;
    Buteo::SyncProfile *m_syncProfile;
    Auth *m_auth;
    RequestGenerator *m_requestGenerator;
    ReplyParser *m_replyParser;
    QNetworkAccessManager m_qnam;
    bool m_syncAborted;
    bool m_syncError;

    // auth related
    int m_accountId;
    QString m_serverUrl;
    QString m_webdavPath;
    QString m_username;
    QString m_password;
    QString m_accessToken;
    bool m_ignoreSslErrors;

    // request queue
    QList<QNetworkReply*> m_requestQueue;
    QHash<QString, SyncCache::Album> m_albums;
    QHash<QString, SyncCache::Photo> m_photos;
};

#endif // NEXTCLOUD_IMAGES_SYNCER_P_H
