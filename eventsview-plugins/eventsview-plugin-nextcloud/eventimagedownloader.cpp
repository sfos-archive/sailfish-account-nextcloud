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

#include "eventimagedownloader.h"
#include "synccacheevents.h"

#include <QtCore/QDebug>
#include <QtQml/QQmlInfo>

NextcloudEventImageDownloader::NextcloudEventImageDownloader(QObject *parent)
    : QObject(parent)
    , m_deferLoad(false)
    , m_eventCache(nullptr)
    , m_accountId(0)
    , m_idempToken(0)
{
}

void NextcloudEventImageDownloader::classBegin()
{
    m_deferLoad = true;
}

void NextcloudEventImageDownloader::componentComplete()
{
    m_deferLoad = false;
    if (m_eventCache) {
        loadImage();
    }
}

SyncCache::EventCache *NextcloudEventImageDownloader::eventCache() const
{
    return m_eventCache;
}

void NextcloudEventImageDownloader::setEventCache(SyncCache::EventCache *cache)
{
    if (m_eventCache == cache) {
        return;
    }

    if (m_eventCache) {
        disconnect(m_eventCache, 0, this, 0);
    }

    m_eventCache = cache;
    emit eventCacheChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

int NextcloudEventImageDownloader::accountId() const
{
    return m_accountId;
}

void NextcloudEventImageDownloader::setAccountId(int id)
{
    if (m_accountId == id) {
        return;
    }

    m_accountId = id;
    emit accountIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QString NextcloudEventImageDownloader::eventId() const
{
    return m_eventId;
}
void NextcloudEventImageDownloader::setEventId(const QString &id)
{
    if (m_eventId == id) {
        return;
    }

    m_eventId = id;
    emit eventIdChanged();

    if (!m_deferLoad) {
        loadImage();
    }
}

QUrl NextcloudEventImageDownloader::imagePath() const
{
    return m_imagePath;
}

void NextcloudEventImageDownloader::loadImage()
{
    if (!m_eventCache) {
        qWarning() << "No event cache, cannot load image";
        return;
    }

    m_idempToken = qHash(QStringLiteral("%1|%2").arg(m_accountId).arg(m_eventId));
    QObject *contextObject = new QObject(this);
    connect(m_eventCache, &SyncCache::EventCache::populateEventImageFinished,
            contextObject, [this, contextObject] (int idempToken, const QString &path) {
        if (m_idempToken == idempToken) {
            contextObject->deleteLater();
            const QUrl imagePath = QUrl::fromLocalFile(path);
            if (m_imagePath != imagePath) {
                m_imagePath = imagePath;
                emit imagePathChanged();
            }
        }
    });
    connect(m_eventCache, &SyncCache::EventCache::populateEventImageFailed,
            contextObject, [this, contextObject] (int idempToken, const QString &errorMessage) {
        if (m_idempToken == idempToken) {
            contextObject->deleteLater();
            qWarning() << "NextcloudEventImageDownloader::loadImage: failed:" << errorMessage;
        }
    });
    m_eventCache->populateEventImage(m_idempToken, m_accountId, m_eventId, QNetworkRequest());
}
