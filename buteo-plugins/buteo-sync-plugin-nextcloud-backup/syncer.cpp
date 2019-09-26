/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "syncer_p.h"
#include "accountauthenticator_p.h"
#include "webdavrequestgenerator_p.h"
#include "xmlreplyparser_p.h"

#include <QtCore/QDateTime>
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QByteArray>
#include <QtCore/QStandardPaths>
#include <QtCore/QCryptographicHash>
#include <QtCore/QMimeDatabase>

// accounts
#include <Accounts/Manager>
#include <Accounts/Account>

// nemo
#include <ssudeviceinfo.h>
#include <MGConfItem>

// buteo
#include <SyncProfile.h>
#include <LogMacros.h>
#include <ProfileEngineDefs.h>

namespace {
    const int HTTP_UNAUTHORIZED_ACCESS = 401;
    const int HTTP_METHOD_NOT_ALLOWED = 405;
    const QString NEXTCLOUD_USERID = QStringLiteral("nextcloud");

    const QString DefaultLocalPath = QStringLiteral("/home/nemo/.local/share/system/privileged/Backups");
    const QString DefaultDirListLocalPath = DefaultLocalPath + "/directoryListing.txt";
}

static QString BackupDirName()
{
    SsuDeviceInfo deviceInfo;
    QString deviceId = deviceInfo.deviceUid();
    if (deviceId.isEmpty()) {
        return QString();
    }

    QByteArray hashedDeviceId = QCryptographicHash::hash(deviceId.toUtf8(), QCryptographicHash::Sha256);
    QString encodedDeviceId = QString::fromUtf8(hashedDeviceId.toBase64(QByteArray::Base64UrlEncoding)).mid(0,12);

    QString deviceDisplayNamePrefix = deviceInfo.displayName(Ssu::DeviceModel);
    if (!deviceDisplayNamePrefix.isEmpty()) {
        deviceDisplayNamePrefix = deviceDisplayNamePrefix.replace(' ', '-') + '_';
    }

    return deviceDisplayNamePrefix + encodedDeviceId;
}

Syncer::Syncer(QObject *parent, Buteo::SyncProfile *syncProfile)
    : QObject(parent)
    , m_syncProfile(syncProfile)
{
}

Syncer::~Syncer()
{
    delete m_auth;
    delete m_requestGenerator;
}

void Syncer::abortSync()
{
    m_syncAborted = true;
}

void Syncer::startSync(int accountId)
{
    Q_ASSERT(accountId != 0);
    m_accountId = accountId;
    delete m_auth;
    m_auth = new AccountAuthenticator("storage", "nextcloud-backup", this);
    connect(m_auth, &AccountAuthenticator::signInCompleted,
            this, &Syncer::sync);
    connect(m_auth, &AccountAuthenticator::signInError,
            this, &Syncer::signInError);
    LOG_DEBUG(Q_FUNC_INFO << "starting Nextcloud backup sync with account" << m_accountId);
    m_auth->signIn(accountId);
}

void Syncer::signInError()
{
    emit syncFailed();
}

bool Syncer::loadConfig()
{
    MGConfItem operationTypeConf("/SailfishOS/vault/nextcloud/operationType");
    if (operationTypeConf.value().toString() == QStringLiteral("sync")) {
        MGConfItem directionConf("/SailfishOS/vault/nextcloud/direction");
        m_operation = (directionConf.value().toString() == Buteo::VALUE_TO_REMOTE) ? Upload : Download;
        directionConf.set(QVariant());
    } else {
        m_operation = List;
    }
    operationTypeConf.set(QVariant());

    const QString &subDirName = BackupDirName();
    if (subDirName.isEmpty()) {
        LOG_WARNING("Could not determine device identifier to create remote-device backup directory!");
        return false;
    } else {
        m_remoteBackupDirPath = "Sailfish OS/Backups/" + subDirName;
    }

    if (m_operation == Upload || m_operation == Download) {
        MGConfItem localPathConf("/SailfishOS/vault/nextcloud/localPath");
        m_localBackupDirPath = localPathConf.value().toString();
        localPathConf.set(QVariant());

        if (m_operation != List && m_localBackupDirPath.isEmpty()) {
            LOG_WARNING("No local directory specified");
            return false;
        }

        if (m_operation == Upload) {
            QDir localBackupDir(m_localBackupDirPath);
            if (!localBackupDir.exists()) {
                LOG_WARNING("Specified local dir" << localBackupDir.absolutePath() << "does not exist!");
                return false;
            }
            m_backupFileNames = localBackupDir.entryList(QDir::Files);
            if (m_backupFileNames.isEmpty()) {
                LOG_WARNING("Upload failed, no files found in" << localBackupDir.absolutePath());
                return false;
            }
            LOG_DEBUG("Will upload files:" << m_backupFileNames);
        } else {
            QDir dir;
            if (!dir.mkpath(m_localBackupDirPath)) {
                LOG_WARNING("Cannot create local dir" << m_localBackupDirPath);
                return false;
            }
            MGConfItem remoteFileConf("/SailfishOS/vault/nextcloud/remoteFile");
            QString remoteFileName = remoteFileConf.value().toString();
            remoteFileConf.set(QVariant());
            if (!remoteFileName.isEmpty()) {
                m_backupFileNames = QStringList(remoteFileName);
            }
        }
    }

    if (m_operation == List) {
        MGConfItem dirListingLocalPathConf("/SailfishOS/vault/nextcloud/listResultLocalPath");
        m_dirListingFileName = dirListingLocalPathConf.value().toString();
        dirListingLocalPathConf.set(QVariant());
        if (m_dirListingFileName.isEmpty()) {
            LOG_WARNING("No text file set for writing directory listing results");
            return false;
        }
    }

    return true;
}

void Syncer::sync(const QString &serverUrl, const QString &webdavPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors)
{
    m_serverUrl = serverUrl;
    m_webdavPath = webdavPath.isEmpty() ? QStringLiteral("/remote.php/webdav/") : webdavPath;
    m_username = username;
    m_password = password;
    m_accessToken = accessToken;
    m_ignoreSslErrors = ignoreSslErrors;

    delete m_requestGenerator;
    m_requestGenerator = accessToken.isEmpty()
                       ? new WebDavRequestGenerator(&m_qnam, username, password)
                       : new WebDavRequestGenerator(&m_qnam, accessToken);

    if (!loadConfig()) {
        finishWithError("Config load failed");
        return;
    }

    LOG_DEBUG("Starting Nextcloud operation:" << m_operation);

    // All operations require an initial directory listing request
    if (!performDirListingRequest(m_remoteBackupDirPath)) {
        finishWithError("Directory list request failed");
        return;
    }
}

bool Syncer::performDirCreationRequest(const QStringList &remotePathParts, int remotePathPartsIndex)
{
    QString remoteDirPath = m_webdavPath + remotePathParts.mid(0, remotePathPartsIndex + 1).join('/');
    QNetworkReply *reply = m_requestGenerator->dirCreation(m_serverUrl, remoteDirPath);
    if (reply) {
        reply->setProperty("remotePathParts", remotePathParts);
        reply->setProperty("remotePathPartsIndex", remotePathPartsIndex);
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleDirCreationReply);
        return true;
    }

    return false;
}

void Syncer::handleDirCreationReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QStringList remotePathParts = reply->property("remotePathParts").toStringList();
    int remotePathPartsIndex = reply->property("remotePathPartsIndex").toInt();

    const bool dirAlreadyExists = (httpCode == HTTP_METHOD_NOT_ALLOWED);
    if (dirAlreadyExists
            || reply->error() == QNetworkReply::NoError) {
        if ((remotePathPartsIndex + 1) >= remotePathParts.length()) {
            // Full directory path has been created. The backup file can now be uploaded.
            performUploadRequest(m_backupFileNames);
        } else {
            // Continue creating the rest of the remote directories in the remote ptah
            performDirCreationRequest(remotePathParts, remotePathPartsIndex + 1);
        }
    } else {
        finishWithError("Dir creation request returned HTTP status: " + httpCode);
        return;
    }
}

bool Syncer::performDirListingRequest(const QString &remoteDirPath)
{
    QNetworkReply *reply = m_requestGenerator->dirListing(m_serverUrl, m_webdavPath + remoteDirPath);
    if (reply) {
        reply->setProperty("remoteDirPath", remoteDirPath);
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleDirListingReply);
        return true;
    }

    return false;
}

void Syncer::handleDirListingReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const QByteArray replyData = reply->readAll();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString remoteDirPath = reply->property("remoteDirPath").toString();

    bool dirNotFound = (httpCode == 404);
    if (m_operation == Upload && dirNotFound) {
        // Remote backup directory doesn't exist, so create it.
        performDirCreationRequest(m_remoteBackupDirPath.split('/'), 0);
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        finishWithHttpError("Remote directory listing failed", httpCode);
        return;
    }

    if (m_operation == List) {
        const QList<XmlReplyParser::Resource> resourceList = XmlReplyParser::parsePropFindResponse(
                replyData, remoteDirPath);
        QFile file(m_dirListingFileName);
        if (!file.open(QFile::WriteOnly | QFile::Text)) {
            finishWithError("Cannot open " + file.fileName() + " for writing!");
            return;
        }
        for (const XmlReplyParser::Resource &resource : resourceList) {
            LOG_DEBUG("Found remote file or dir:" << resource.href);
            if (!resource.isCollection
                    && file.write(resource.href.toUtf8() + '\n') < 0) {
                finishWithError("Failed to write to " + file.fileName());
                return;
            }
        }
        file.close();
        finishWithSuccess();

    } else if (m_operation == Upload) {
        // Remote backup directory exists, so upload the file
        if (!performUploadRequest(m_backupFileNames)) {
            finishWithError("Upload request failed");
        }

    } else if (m_operation == Download) {
        const QList<XmlReplyParser::Resource> resourceList = XmlReplyParser::parsePropFindResponse(
                replyData, remoteDirPath);
        for (const XmlReplyParser::Resource &resource : resourceList) {
            if (!resource.isCollection) {
                int lastDirSep = resource.href.lastIndexOf('/');
                QString fileName = resource.href.mid(lastDirSep + 1);
                if (fileName.length() > 0) {
                    m_backupFileNames << fileName;
                }
            }
            LOG_DEBUG("Will download files:" << m_backupFileNames);
        }
        if (m_backupFileNames.isEmpty()) {
            LOG_WARNING("No files found to download from remote directory:" << remoteDirPath);
            finishWithSuccess();
        } else if (!performDownloadRequest(m_backupFileNames)) {
            finishWithError("Dir list request failed");
        }

    } else {
        finishWithError("Unexpected operation after dir listing finished: " + m_operation);
    }
}

bool Syncer::performUploadRequest(const QStringList &fileNameList)
{
    if (fileNameList.isEmpty()) {
        LOG_WARNING("No files to upload!");
        return false;
    }

    const QString localFilePath = m_localBackupDirPath + '/' + fileNameList.at(0);
    QFile file(localFilePath);
    if (!file.open(QFile::ReadOnly)) {
        LOG_WARNING("Cannot open local file to be uploaded:" << file.fileName());
        return false;
    }
    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForData(&file);
    const QByteArray fileData = file.readAll();
    file.close();

    const QString remotePath = m_webdavPath + m_remoteBackupDirPath + '/' + fileNameList.at(0);
    QNetworkReply *reply = m_requestGenerator->upload(m_serverUrl, mimeType.name(), fileData, remotePath);
    if (reply) {
        reply->setProperty("fileNameList", QVariant(fileNameList.mid(1)));

        connect(reply, &QNetworkReply::uploadProgress,
                this, &Syncer::handleUploadProgress);
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleUploadReply);
        return true;
    }

    return false;
}

void Syncer::handleUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    LOG_DEBUG("Nextcloud uploaded" << bytesSent << "bytes of" << bytesTotal);
}

void Syncer::handleUploadReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        finishWithHttpError("Upload failed", httpCode);
        return;
    }

    QStringList fileNameList = reply->property("fileNameList").toStringList();
    if (fileNameList.isEmpty()) {
        finishWithSuccess();
    } else {
        performUploadRequest(fileNameList);
    }
}

bool Syncer::performDownloadRequest(const QStringList &fileNameList)
{
    if (fileNameList.isEmpty()) {
        LOG_WARNING("No files to download!");
        return false;
    }

    const QString localFilePath = m_localBackupDirPath + '/' + fileNameList.at(0);
    if (m_downloadedFile) {
        delete m_downloadedFile;
    }
    m_downloadedFile = new QFile(localFilePath);
    if (!m_downloadedFile->open(QFile::WriteOnly)) {
        LOG_WARNING("Cannot open file for writing: " + m_downloadedFile->fileName());
        delete m_downloadedFile;
        m_downloadedFile = nullptr;
        return false;
    }

    const QString remoteFilePath = m_webdavPath + m_remoteBackupDirPath + '/' + fileNameList.at(0);
    QNetworkReply *reply = m_requestGenerator->download(m_serverUrl, remoteFilePath);
    if (reply) {
        reply->setProperty("fileNameList", QVariant(fileNameList.mid(1)));
        connect(reply, &QNetworkReply::downloadProgress,
                this, &Syncer::handleDownloadProgress);
        connect(reply, &QNetworkReply::finished,
                this, &Syncer::handleDownloadReply);
        return true;
    }

    return false;
}

void Syncer::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    LOG_DEBUG("Nextcloud downloaded" << bytesReceived << "bytes of" << bytesTotal);

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if (!m_downloadedFile) {
        LOG_WARNING("Download file is not set!");
        reply->abort();
        return;
    }

    if (reply->bytesAvailable() > 0
            && m_downloadedFile->write(reply->readAll()) < 0) {
        LOG_WARNING("Failed to write" << reply->bytesAvailable()
                    << "bytes to file:" << m_downloadedFile->fileName());
        reply->abort();
    }
}

void Syncer::handleDownloadReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    const int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (m_downloadedFile) {
        m_downloadedFile->close();
        delete m_downloadedFile;
        m_downloadedFile = nullptr;
    }

    if (reply->error() != QNetworkReply::NoError) {
        finishWithHttpError("Download failed", httpCode);
        return;
    }

    QStringList fileNameList = reply->property("fileNameList").toStringList();
    if (fileNameList.isEmpty()) {
        finishWithSuccess();
    } else {
        performDownloadRequest(fileNameList);
    }
}

void Syncer::finishWithHttpError(const QString &errorMessage, int httpCode)
{
    if (httpCode == HTTP_UNAUTHORIZED_ACCESS) {
        m_auth->setCredentialsNeedUpdate(m_accountId);
    }
    finishWithError(QString("%1 (http status=%2)").arg(errorMessage).arg(httpCode));
}

void Syncer::finishWithError(const QString &errorMessage)
{
    LOG_WARNING("Nextcloud backup sync for account" << m_accountId << "finished with error:" << errorMessage);
    m_syncError = true;
    emit syncFailed();
}

void Syncer::finishWithSuccess()
{
    LOG_DEBUG(Q_FUNC_INFO << "Nextcloud dir listing with account" << m_accountId << "finished successfully!");
    emit syncSucceeded();
}

void Syncer::purgeAccount(int accountId)
{
    Q_UNUSED(accountId);
}
