/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Gallery 1.0
import com.jolla.gallery 1.0
import com.jolla.gallery.nextcloud 1.0

Page {
    id: fullscreenPage

    property alias accountId: imageDownloader.accountId
    property alias userId: imageDownloader.userId
    property alias albumId: imageDownloader.albumId
    property alias photoId: imageDownloader.photoId

    property NextcloudImageDownloader imgDownloader: NextcloudImageDownloader {
        id: imageDownloader
        imageCache: NextcloudImageCache
        downloadImage: true
    }

    ImageViewer {
        anchors.fill: parent
        source: imageDownloader.imagePath
    }
}
