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

#ifndef NEXTCLOUD_IMAGES_SYNCER_P_H
#define NEXTCLOUD_IMAGES_SYNCER_P_H

#include "webdavsyncer_p.h"
#include "replyparser_p.h"
#include "synccacheimages.h"

// libaccounts-qt5
#include <Accounts/Manager>

#include <QList>
#include <QHash>
#include <QVector>

class QNetworkReply;

class ReplyParser;
class JsonRequestGenerator;

class Syncer : public WebDavSyncer
{
    Q_OBJECT

public:
    Syncer(QObject *parent, Buteo::SyncProfile *profile);
   ~Syncer();

    void purgeAccount(int accountId) override;

private:
    void handleUserInfoReply();
    bool performDirListingRequest(const QString &remoteDirPath);
    void handleDirListingReply();

    void purgeDeletedAccounts();
    void deleteFilesForAccount(int accountId);

    void beginSync() override;

    bool processQueriedAlbum(const SyncCache::Album &mainAlbum,
                             const QVector<SyncCache::Photo> &photos,
                             const QVector<SyncCache::Album> &subAlbums);

    bool calculateAndApplyDelta(const SyncCache::Album &mainAlbum,
                                const QVector<SyncCache::Photo> &photos,
                                const QVector<SyncCache::Album> &subAlbums,
                                SyncCache::ImageDatabase *db,
                                SyncCache::DatabaseError *error);

    class SyncProgressInfo
    {
    public:
        void reset();

        int addedAlbumCount = 0;
        int modifiedAlbumCount = 0;
        int removedAlbumCount = 0;

        int addedPhotoCount = 0;
        int modifiedPhotoCount = 0;
        int removedPhotoCount = 0;

        QStringList pendingAlbumListings;
    };

    SyncProgressInfo m_syncProgressInfo;
    Accounts::Manager *m_manager = nullptr;
    ReplyParser *m_replyParser = nullptr;
    QString m_userId;
    QString m_dirListingRootPath;
    bool m_forceFullSync = false;
};

#endif // NEXTCLOUD_IMAGES_SYNCER_P_H
