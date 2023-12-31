/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Nextcloud account package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

#ifndef NEXTCLOUDUPLOADER_H
#define NEXTCLOUDUPLOADER_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

// transferengine-qt5
#include <mediatransferinterface.h>

#include "nextcloudshareservicestatus.h"

class NextcloudApi;
class AccountSettings;
class QFile;

class NextcloudUploader : public MediaTransferInterface
{
    Q_OBJECT

public:
    NextcloudUploader(QNetworkAccessManager *qnam, QObject *parent = 0);
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

private:
    void postFile();
    void cleanUp();

    NextcloudApi *m_api = nullptr;
    NextcloudShareServiceStatus *m_nextcloudShareServiceStatus = nullptr;
    QNetworkAccessManager *m_qnam = nullptr;
    QFile *m_contentFile = nullptr;
    NextcloudShareServiceStatus::AccountDetails m_accountDetails;
    QString m_filePath;
    QString m_token;

    friend class ut_nextclouduploader;
};

#endif // NEXTCLOUDUPLOADER_H
