/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_IMAGES_SYNCER_P_H
#define NEXTCLOUD_IMAGES_SYNCER_P_H

#include "webdavsyncer_p.h"
#include "replyparser_p.h"
#include "synccacheimages.h"

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

    void beginSync() override;
    void calculateAndApplyDelta(const QHash<QString, SyncCache::Album> &albums,
                                const QHash<QString, SyncCache::Photo> &photos,
                                const QString &firstPhotoId);

    ReplyParser *m_replyParser = nullptr;
    ReplyParser::GalleryMetadata m_dirListingResults;
    QStringList m_pendingAlbumListings;
    QString m_userId;
    QString m_dirListingRootPath;
};

#endif // NEXTCLOUD_IMAGES_SYNCER_P_H
