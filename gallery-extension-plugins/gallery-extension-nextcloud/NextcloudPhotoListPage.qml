/****************************************************************************************
** Copyright (c) 2019 - 2020 Open Mobile Platform LLC.
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
import Sailfish.Gallery 1.0
import Sailfish.FileManager 1.0
import com.jolla.gallery 1.0
import com.jolla.gallery.nextcloud 1.0

Page {
    id: root

    property alias accountId: photoModel.accountId
    property alias userId: photoModel.userId
    property alias albumId: photoModel.albumId
    property string albumName

    NextcloudPhotoModel {
        id: photoModel

        imageCache: NextcloudImageCache
    }

    SilicaListView {
        anchors.fill: parent
        model: photoModel

        header: PageHeader {
            title: {
                if (albumName.length > 0) {
                    var lastSlash = albumName.lastIndexOf('/')
                    return lastSlash >= 0 ? albumName.substring(lastSlash + 1) : albumName
                }
                //: Heading for Nextcloud photos
                //% "Nextcloud"
                return qsTrId("jolla_gallery_nextcloud-la-nextcloud")
            }
        }

        delegate: BackgroundItem {
            id: photoDelegate

            width: parent.width
            height: fileItem.height

            onClicked: {
                var props = {
                    "imageModel": photoModel,
                    "currentIndex": model.index
                }
                pageStack.push(Qt.resolvedUrl("NextcloudFullscreenPhotoPage.qml"), props)
            }

            FileItem {
                id: fileItem

                fileName: model.fileName
                mimeType: model.fileType
                size: model.fileSize
                isDir: false
                created: model.createdTimestamp
                modified: model.modifiedTimestamp

                icon {
                    source: thumbDownloader.status === NextcloudImageDownloader.Ready
                            ? thumbDownloader.imagePath
                            : Theme.iconForMimeType(model.fileType)
                    width: thumbDownloader.status === NextcloudImageDownloader.Ready
                           ? Theme.itemSizeMedium
                           : undefined
                    height: icon.width
                    sourceSize.width: icon.width
                    sourceSize.height: icon.width
                    clip: thumbDownloader.status === NextcloudImageDownloader.Ready
                    fillMode: Image.PreserveAspectCrop
                    highlighted: thumbDownloader.status !== NextcloudImageDownloader.Ready && photoDelegate.highlighted
                    opacity: thumbDownloader.status === NextcloudImageDownloader.Ready && photoDelegate.highlighted ? Theme.opacityHigh : 1
                }
            }

            NextcloudImageDownloader {
                id: thumbDownloader

                imageCache: NextcloudImageCache
                downloadThumbnail: true
                accountId: model.accountId
                userId: model.userId
                albumId: model.albumId
                photoId: model.photoId
            }
        }
    }
}
