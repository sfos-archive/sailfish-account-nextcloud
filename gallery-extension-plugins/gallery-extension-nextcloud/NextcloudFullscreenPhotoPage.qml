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

import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.Silica.private 1.0 as SilicaPrivate
import Sailfish.Gallery 1.0
import com.jolla.gallery 1.0
import com.jolla.gallery.nextcloud 1.0

FullscreenContentPage {
    id: root

    property var imageModel
    property alias currentIndex: slideshowView.currentIndex

    allowedOrientations: Orientation.All

    // Update the Cover via window.activeObject property
    Binding {
        target: window
        property: "activeObject"
        property bool active: root.status === PageStatus.Active
        value: { "url": active ? slideshowView.currentItem.source : "", "mimeType": active ? slideshowView.currentItem.mimeType : "" }
    }

    SlideshowView {
        id: slideshowView

        anchors.fill: parent
        itemWidth: width
        itemHeight: height

        model: root.imageModel

        delegate: ImageViewer {
            id: delegateItem

            readonly property string mimeType: model.fileType
            readonly property bool downloading: imageDownloader.status === NextcloudImageDownloader.Downloading

            width: slideshowView.width
            height: slideshowView.height

            source: imageDownloader.status === NextcloudImageDownloader.Ready
                     ? imageDownloader.imagePath
                     : ""
            active: PathView.isCurrentItem
            viewMoving: slideshowView.moving

            onZoomedChanged: overlay.active = !zoomed
            onClicked: {
                if (zoomed) {
                    zoomOut()
                } else {
                    overlay.active = !overlay.active
                }
            }

            InfoLabel {
                //% "Image download failed"
                text: qsTrId("jolla_gallery_nextcloud-la-image_download_failed")
                anchors.centerIn: parent
                visible: imageDownloader.status === NextcloudImageDownloader.Error
            }

            NextcloudImageDownloader {
                id: imageDownloader

                accountId: model.accountId
                userId: model.userId
                albumId: model.albumId
                photoId: model.photoId

                imageCache: NextcloudImageCache
                downloadImage: delegateItem.active
            }
        }
    }

    GalleryOverlay {
        id: overlay

        anchors.fill: parent

        // Currently, images are only downloaded and never uploaded, so
        // don't allow editing or deleting.
        deletingAllowed: false
        editingAllowed: false

        source: slideshowView.currentItem ? slideshowView.currentItem.source : ""
        isImage: true
        duration: 1
        error: slideshowView.currentItem && slideshowView.currentItem.error
    }

    IconButton {
        id: detailsButton
        x: Theme.horizontalPageMargin
        y: Theme.paddingLarge
        icon.source: "image://theme/icon-m-about"
        onClicked: {
            var props = {
                "modelData": imageModel.at(root.currentIndex),
                "imageMetaData": slideshowView.currentItem.imageMetaData
            }
            pageStack.animatorPush("NextcloudImageDetailsPage.qml", props)
        }
    }

    SilicaPrivate.DismissButton {}

    BusyIndicator {
        anchors.centerIn: parent
        size: BusyIndicatorSize.Large
        running: slideshowView.currentItem && slideshowView.currentItem.downloading
    }
}
