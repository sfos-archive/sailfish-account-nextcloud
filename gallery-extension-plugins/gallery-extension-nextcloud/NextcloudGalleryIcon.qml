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
import com.jolla.gallery 1.0
import com.jolla.gallery.nextcloud 1.0

MediaSourceIcon {
    id: root

    property bool _showPlaceholder

    // shuffle thumbnails with timer similar to that of gallery photos but without aligning exactly
    timerEnabled: allPhotosModel.count > 1
    timerInterval: (!slideShow.currentItem || slideShow.currentItem.status === Image.Null)
                   ? 200
                   : Math.floor(Math.random() * 8000) + 6000

    onTimerTriggered: {
        var startIndex = (slideShow.currentIndex + 1) % allPhotosModel.count
        for (var i = startIndex; i < allPhotosModel.count; ++i) {
            var data = allPhotosModel.at(i)
            if (data.thumbnailPath.toString().length > 0) {
                slideShow.currentIndex = i
                break
            }
        }
    }

    ListView {
        id: slideShow

        interactive: false
        currentIndex: 0
        clip: true
        orientation: ListView.Horizontal
        cacheBuffer: width * 2
        anchors.fill: parent

        model: allPhotosModel

        delegate: Image {
            id: thumbnail

            source: model.thumbnailPath
            width: slideShow.width
            height: slideShow.height
            sourceSize.width: slideShow.width
            sourceSize.height: slideShow.height
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
    }

    NextcloudPhotosModel {
        id: allPhotosModel

        imageCache: NextcloudImageCache
    }

    NextcloudUsersModel {
        id: nextcloudUsers

        imageCache: NextcloudImageCache
    }

    Timer {
        id: loadingTimer
        running: true
        interval: 1000
        onTriggered: {
            if (nextcloudUsers.count === 0) {
                root._showPlaceholder = true
            }
        }
    }

    Image {
        anchors.fill: parent
        source: "image://theme/graphic-service-nextcloud"
        fillMode: Image.PreserveAspectCrop
        clip: true
        visible: (_showPlaceholder && nextcloudUsers.count === 0)
                 || (slideShow.currentItem && (slideShow.currentItem.status === Image.Null || slideShow.currentItem.status === Image.Error))
    }
}
