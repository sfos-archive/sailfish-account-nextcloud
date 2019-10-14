/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_WEBDAVSYNCER_P_H
#define NEXTCLOUD_WEBDAVSYNCER_P_H

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class AccountAuthenticator;
class WebDavRequestGenerator;
namespace Buteo { class SyncProfile; }

class WebDavSyncer : public QObject
{
    Q_OBJECT

public:
    WebDavSyncer(QObject *parent, Buteo::SyncProfile *profile, const QString &serviceName);
   ~WebDavSyncer();

    void startSync(int accountId);
    virtual void abortSync();
    virtual void purgeAccount(int accountId) = 0;

Q_SIGNALS:
    void syncSucceeded();
    void syncFailed();

protected Q_SLOTS:
    void sync(int accountId, const QString &serviceName,
              const QString &serverUrl, const QString &webdavPath,
              const QString &username, const QString &password,
              const QString &accessToken, bool ignoreSslErrors);
    void signInError();

protected:
    virtual void beginSync() = 0;
    void finishWithHttpError(const QString &errorMessage, int httpCode);
    void finishWithError(const QString &errorMessage);
    void finishWithSuccess();

    Buteo::SyncProfile *m_syncProfile = nullptr;
    AccountAuthenticator *m_auth = nullptr;
    WebDavRequestGenerator *m_requestGenerator = nullptr;
    QNetworkAccessManager m_qnam;
    bool m_syncAborted = false;
    bool m_syncError = false;

    // auth related
    int m_accountId = 0;
    QString m_serviceName;
    QString m_serverUrl;
    QString m_webdavPath;
    QString m_username;
    QString m_password;
    QString m_accessToken;
    bool m_ignoreSslErrors = false;
};

#endif // NEXTCLOUD_WEBDAVSYNCER_P_H
