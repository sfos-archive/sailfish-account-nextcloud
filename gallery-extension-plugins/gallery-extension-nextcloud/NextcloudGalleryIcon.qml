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

import QtQuick 2.0
import Sailfish.Silica 1.0
import com.jolla.gallery 1.0
import com.jolla.gallery.nextcloud 1.0

MediaSourceIcon {
    id: root

    property bool _showPlaceholder

    // shuffle thumbnails with timer similar to that of gallery photos but without aligning exactly
    timerEnabled: allPhotosModel.count > 1
    timerInterval: Math.floor(Math.random() * 8000) + 6000

    onTimerTriggered: _showNextValidPhoto()

    function _showNextValidPhoto() {
        var startIndex = (slideShow.currentIndex + 1) % allPhotosModel.count
        var restarted = false

        // Loop until a downloaded thumbnail is found.
        for (var i = startIndex; i < allPhotosModel.count; ++i) {
            var data = allPhotosModel.at(i)
            if (data.thumbnailPath.toString().length > 0) {
                slideShow.currentIndex = i
                break
            }
            if (restarted && i === startIndex) {
                // There are no downloaded thumbnails yet
                root._showPlaceholder = true
                break
            } else if (i === allPhotosModel.count - 1) {
                restarted = true
                i = -1
            }
        }
    }

    ListView {
        id: slideShow

        anchors.fill: parent
        interactive: false
        currentIndex: -1
        clip: true
        orientation: ListView.Horizontal
        cacheBuffer: width * 2
        highlightMoveDuration: 0    // jump immediately to initial image instead of animating

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

    NextcloudPhotoModel {
        id: allPhotosModel

        imageCache: NextcloudImageCache

        onCountChanged: {
            if (slideShow.currentIndex < 0) {
                _showNextValidPhoto()
                slideShow.highlightMoveDuration = -1    // restore animation for cycling to next image
            }
        }
    }

    NextcloudUserModel {
        id: nextcloudUsers

        imageCache: NextcloudImageCache
    }

    Image {
        anchors.fill: parent
        source: "image://theme/graphic-service-nextcloud"
        fillMode: Image.PreserveAspectCrop
        clip: true
        visible: _showPlaceholder
                 || (slideShow.currentItem
                     && (slideShow.currentItem.status === Image.Null || slideShow.currentItem.status === Image.Error))
    }
}
