/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUDAPI_H
#define NEXTCLOUDAPI_H

#include <QtCore/QObject>

class NextcloudApi : public QObject
{
    Q_OBJECT

public:
    enum API_CALL {
        NONE,
        REQUEST_FOLDER,
        CREATE_FOLDER,
        UPLOAD_FILE,
    };

    NextcloudApi(QObject *parent=0);
    ~NextcloudApi();

    void uploadFile(const QString &filePath,
                    const QVariantMap &nextcloudParameters);
    void cancelUpload();

protected Q_SLOTS:
    void finished(QNetworkReply *reply);
    void networkError(QNetworkReply::NetworkError error);
    void uploadProgress(qint64 received, qint64 total);
    void timedOut();

private:
    qint64 m_fileSize;
    qint64 m_transferred;
    QMap<QNetworkReply*, API_CALL> m_replies;
    QMap<QTimer*, QNetworkReply*> m_timeouts;
};

#endif // NEXTCLOUDAPI_H
