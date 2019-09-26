/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_BACKUP_SYNCER_P_H
#define NEXTCLOUD_BACKUP_SYNCER_P_H

#include <QObject>
#include <QDateTime>
#include <QString>
#include <QList>
#include <QHash>
#include <QPair>
#include <QNetworkAccessManager>

class AccountAuthenticator;
class WebDavRequestGenerator;
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
    enum Operation {
        List,
        Upload,
        Download
    };

    bool performDirCreationRequest(const QStringList &remotePathParts, int remotePathPartsIndex);
    void handleDirCreationReply();

    bool performDirListingRequest(const QString &remoteDirPath);
    void handleDirListingReply();

    bool performUploadRequest(const QStringList &fileNameList);
    void handleUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void handleUploadReply();

    bool performDownloadRequest(const QStringList &fileNameList);
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleDownloadReply();

    void finishWithHttpError(const QString &errorMessage, int httpCode);
    void finishWithError(const QString &errorMessage);
    void finishWithSuccess();

    bool loadConfig();

    Buteo::SyncProfile *m_syncProfile = nullptr;
    AccountAuthenticator *m_auth = nullptr;
    WebDavRequestGenerator *m_requestGenerator = nullptr;
    QFile *m_downloadedFile = nullptr;
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

    QString m_localBackupDirPath;
    QString m_remoteBackupDirPath;
    QStringList m_backupFileNames;
    QString m_dirListingFileName;
    Operation m_operation;
};

#endif // NEXTCLOUD_BACKUP_SYNCER_P_H
