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
#include <QtDebug>

NextcloudUploader::NextcloudUploader(QObject *parent)
    : MediaTransferInterface(parent)
    , m_nextcloudShareServiceStatus(0)
    , m_api(0)
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
    return QUrl("image://theme/icon-s-service-nextcloud");
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
        connect(m_nextcloudShareServiceStatus, SIGNAL(serviceReady()), this, SLOT(startUploading()));
        connect(m_nextcloudShareServiceStatus, SIGNAL(credentialsCleared()), this, SLOT(transferError()));
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
    quint32 accountId = mediaItem()->value(MediaItem::AccountId).toInt();
    QVariantMap details = m_nextcloudShareServiceStatus->detailsByIdentifier(accountId);
    if (details.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Nextcloud account details are empty. Can't start uploading!";
        return;
    }

    m_detailsMap = details;

    const QString mimeType = mediaItem()->value(MediaItem::MimeType).toString();
    if (mimeType.contains("image") || mimeType.contains("video")) {
        postFile();
    } else {
        qWarning() << Q_FUNC_INFO << "Undefined mimeType: " << (mimeType.isEmpty() ? "empty" : mimeType);
    }
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
    m_nextcloudShareServiceStatus->setCredentialsExpired(accountId);
}

void NextcloudUploader::postFile()
{
    m_filePath = mediaItem()->value(MediaItem::Url).toUrl().toLocalFile();
    createApi();

    setStatus(MediaTransferInterface::TransferStarted);
    m_api->uploadFile(m_filePath, m_detailsMap);
}

void NextcloudUploader::createApi()
{
    if (!m_api) {
        m_api = new NextcloudApi(this);
        connect(m_api, SIGNAL(transferProgressUpdated(qreal)), this, SLOT(transferProgress(qreal)));
        connect(m_api, SIGNAL(transferFinished()), this, SLOT(transferFinished()));
        connect(m_api, SIGNAL(transferError()), this, SLOT(transferError()));
        connect(m_api, SIGNAL(transferCanceled()), this, SLOT(transferCanceled()));
        connect(m_api, SIGNAL(credentialsExpired()), this, SLOT(credentialsExpired()));
    }
}
