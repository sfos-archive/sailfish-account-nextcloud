/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_POSTS_SYNCER_P_H
#define NEXTCLOUD_POSTS_SYNCER_P_H

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QList>
#include <QHash>
#include <QPair>
#include <QNetworkAccessManager>

class AccountAuthenticator;
class WebDavRequestGenerator;
class XmlReplyParser;
class QFile;
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
    void signInError();

private:
    bool performCapabilitiesRequest();
    void handleCapabilitiesReply();

    bool performNotificationListRequest();
    void handleNotificationListReply();

    void finishWithHttpError(const QString &errorMessage, int httpCode);
    void finishWithError(const QString &errorMessage);
    void finishWithSuccess();

    friend class WebDavRequestGenerator;

    Buteo::SyncProfile *m_syncProfile = nullptr;
    AccountAuthenticator *m_auth = nullptr;
    WebDavRequestGenerator *m_requestGenerator = nullptr;
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
};

#endif // NEXTCLOUD_POSTS_SYNCER_P_H
