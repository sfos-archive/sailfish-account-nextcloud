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

#ifndef NEXTCLOUD_EVENTSVIEW_EVENTIMAGEDOWNLOADER_H
#define NEXTCLOUD_EVENTSVIEW_EVENTIMAGEDOWNLOADER_H

#include "eventcache.h"

#include <QtQml/QQmlParserStatus>
#include <QtCore/QUrl>

class NextcloudEventImageDownloader : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::EventCache* eventCache READ eventCache WRITE setEventCache NOTIFY eventCacheChanged)
    Q_PROPERTY(int accountId READ accountId WRITE setAccountId NOTIFY accountIdChanged)
    Q_PROPERTY(QString eventId READ eventId WRITE setEventId NOTIFY eventIdChanged)
    Q_PROPERTY(QUrl imagePath READ imagePath NOTIFY imagePathChanged)

public:
    explicit NextcloudEventImageDownloader(QObject *parent = nullptr);

    // QQmlParserStatus
    void classBegin() override;
    void componentComplete() override;

    SyncCache::EventCache *eventCache() const;
    void setEventCache(SyncCache::EventCache *cache);

    int accountId() const;
    void setAccountId(int id);

    QString eventId() const;
    void setEventId(const QString &eventId);

    QUrl imagePath() const;

Q_SIGNALS:
    void eventCacheChanged();
    void accountIdChanged();
    void eventIdChanged();
    void imagePathChanged();

private:
    void loadImage();

    bool m_deferLoad;
    SyncCache::EventCache *m_eventCache;
    int m_accountId;
    QString m_eventId;
    QUrl m_imagePath;
    int m_idempToken;
};

#endif // NEXTCLOUD_EVENTSVIEW_EVENTIMAGEDOWNLOADER_H
