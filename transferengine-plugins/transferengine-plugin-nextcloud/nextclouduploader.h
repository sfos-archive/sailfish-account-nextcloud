/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUDUPLOADER_H
#define NEXTCLOUDUPLOADER_H

#include <QtNetwork/QNetworkReply>

// transferengine-qt5
#include <mediatransferinterface.h>

class NextcloudApi;
class AccountSettings;
class NextcloudShareServiceStatus;

class NextcloudUploader : public MediaTransferInterface
{
    Q_OBJECT

public:
    NextcloudUploader(QObject *parent = 0);
    ~NextcloudUploader();

    QString displayName() const;
    QUrl serviceIcon() const;
    bool cancelEnabled() const;
    bool restartEnabled() const;

public Q_SLOTS:
    void start();
    void cancel();

private Q_SLOTS:
    void startUploading();
    void transferFinished();
    void transferProgress(qreal progress);
    void transferError();
    void transferCanceled();
    void credentialsExpired();

protected:
    void postFile();
    void createApi();

private:
    NextcloudShareServiceStatus *m_nextcloudShareServiceStatus;
    NextcloudApi *m_api;
    QString m_filePath;
    QVariantMap m_detailsMap;
    QString m_token;

    friend class ut_nextclouduploader;
};

#endif // NEXTCLOUDUPLOADER_H
