/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextclouduploader.h"
#include "nextcloudshareservicestatus.h"
#include "nextcloudapi.h"

// transferengine-qt5
#include <mediaitem.h>
#include <imageoperation.h>

#include <QFile>
#include <QMimeDatabase>
#include <QMimeData>
#include <QMimeType>
#include <QtDebug>

NextcloudUploader::NextcloudUploader(QNetworkAccessManager *qnam, QObject *parent)
    : MediaTransferInterface(parent)
    , m_api(0)
    , m_nextcloudShareServiceStatus(0)
    , m_qnam(qnam)
{
}

NextcloudUploader::~NextcloudUploader()
{
}

QString NextcloudUploader::displayName() const
{
    return tr("Nextcloud");
}

QUrl NextcloudUploader::serviceIcon() const
{
    return QUrl("image://theme/graphic-s-service-nextcloud");
}

bool NextcloudUploader::cancelEnabled() const
{
    return true;
}

bool NextcloudUploader::restartEnabled() const
{
    return true;
}

void NextcloudUploader::start()
{
    // This is the point where we would actually send the file for update
    if (!mediaItem()) {
        qWarning() << Q_FUNC_INFO << "NULL MediaItem. Can't continue";
        setStatus(MediaTransferInterface::TransferInterrupted);
        return;
    }

    // Query status and get access token within the response
    if (!m_nextcloudShareServiceStatus) {
        m_nextcloudShareServiceStatus = new NextcloudShareServiceStatus(this);
        connect(m_nextcloudShareServiceStatus, &NextcloudShareServiceStatus::serviceReady, this, &NextcloudUploader::startUploading);
    }
    m_nextcloudShareServiceStatus->queryStatus();
}

void NextcloudUploader::cancel()
{
    if (m_api != 0) {
        m_api->cancelUpload();
    } else {
        qWarning() << Q_FUNC_INFO << "Can't cancel. NULL Nextcloud API object!";
    }
}

void NextcloudUploader::startUploading()
{
    if (!m_nextcloudShareServiceStatus) {
        qWarning() << Q_FUNC_INFO << "NULL NextcloudShareServiceStatus object!";
        return;
    }

    // In a case of having multiple accounts get the token based on account id
    const quint32 accountId = mediaItem()->value(MediaItem::AccountId).toInt();
    m_accountDetails = m_nextcloudShareServiceStatus->detailsByIdentifier(accountId);
    postFile();
}

void NextcloudUploader::transferFinished()
{
    setStatus(MediaTransferInterface::TransferFinished);
}

void NextcloudUploader::transferProgress(qreal progress)
{
    setProgress(progress);
}

void NextcloudUploader::transferError()
{
    setStatus(MediaTransferInterface::TransferInterrupted);
    qWarning() << Q_FUNC_INFO << "Transfer interrupted";
}

void NextcloudUploader::transferCanceled()
{
    setStatus(MediaTransferInterface::TransferCanceled);
    qWarning() << Q_FUNC_INFO << "Transfer canceled";
}

void NextcloudUploader::credentialsExpired()
{
    quint32 accountId = mediaItem()->value(MediaItem::AccountId).toInt();
    m_nextcloudShareServiceStatus->setCredentialsNeedUpdate(accountId, QStringLiteral("nextcloud-sharing"));
}

void NextcloudUploader::postFile()
{
    m_filePath = mediaItem()->value(MediaItem::Url).toUrl().toLocalFile();
    if (!m_api) {
        m_api = new NextcloudApi(m_qnam, this);
        connect(m_api, &NextcloudApi::transferProgressUpdated, this, &NextcloudUploader::transferProgress);
        connect(m_api, &NextcloudApi::transferFinished, this, &NextcloudUploader::transferFinished);
        connect(m_api, &NextcloudApi::transferError, this, &NextcloudUploader::transferError);
        connect(m_api, &NextcloudApi::transferCanceled, this, &NextcloudUploader::transferCanceled);
        connect(m_api, &NextcloudApi::credentialsExpired, this, &NextcloudUploader::credentialsExpired);
    }
    setStatus(MediaTransferInterface::TransferStarted);

    QMimeDatabase mimeDb;
    QMimeType mimeType = mimeDb.mimeTypeForFile(m_filePath);
    const bool mimeTypeIsImage = mimeType.name().startsWith(QStringLiteral("image"), Qt::CaseInsensitive);

    QObject *contextObj = new QObject(this);
    connect(m_api, &NextcloudApi::propfindFinished, contextObj, [this, contextObj, mimeTypeIsImage] {
        contextObj->deleteLater();
        m_api->uploadFile(m_filePath,
                          m_accountDetails.accessToken,
                          m_accountDetails.username,
                          m_accountDetails.password,
                          m_accountDetails.serverAddress,
                          mimeTypeIsImage ? m_accountDetails.photosPath : m_accountDetails.documentsPath,
                          m_accountDetails.ignoreSslErrors);
    });
    m_api->propfindPath(m_accountDetails.accessToken,
                        m_accountDetails.username,
                        m_accountDetails.password,
                        m_accountDetails.serverAddress,
                        mimeTypeIsImage ? m_accountDetails.photosPath : m_accountDetails.documentsPath);
}
