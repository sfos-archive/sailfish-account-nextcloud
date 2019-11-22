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

Page {
    id: root
    allowedOrientations: window.allowedOrientations

    property alias model: view.model
    property string title

    SilicaListView {
        id: view
        anchors.fill: parent
        header: PageHeader { title: root.title }

        delegate: BackgroundItem {
            id: delegateItem

            height: thumbnail.height

            NextcloudAlbumModel {
                id: nextcloudAlbums

                imageCache: NextcloudImageCache
                accountId: model.accountId
                userId: model.userId
            }

            NextcloudPhotoModel {
                id: photoModel

                imageCache: NextcloudImageCache
                accountId: model.accountId
                userId: model.userId
            }

            NextcloudImageDownloader {
                id: imageDownloader

                accountId: model.accountId
                userId: model.userId

                imageCache: NextcloudImageCache
                downloadThumbnail: true
            }

            Label {
                id: titleLabel

                anchors {
                    left: parent.left
                    leftMargin: Theme.paddingLarge
                    right: thumbnail.left
                    rightMargin: Theme.paddingLarge
                    verticalCenter: parent.verticalCenter
                }
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                font.pixelSize: Theme.fontSizeLarge
                text: model.displayName
            }

            HighlightImage {
                id: thumbnail

                anchors.left: parent.horizontalCenter
                width: Theme.itemSizeExtraLarge
                height: width
                source: imageDownloader.status === NextcloudImageDownloader.Ready
                        ? imageDownloader.imagePath
                        : "image://theme/icon-l-nextcloud"
                fillMode: imageDownloader.status === NextcloudImageDownloader.Ready
                          ? Image.PreserveAspectCrop
                          : Image.PreserveAspectFit
                clip: true
                highlighted: imageDownloader.status !== NextcloudImageDownloader.Ready
                             && delegateItem.highlighted
                opacity: imageDownloader.status === NextcloudImageDownloader.Ready
                         && delegateItem.highlighted
                         ? Theme.opacityHigh
                         : 1
            }

            Label {
                id: countLabel
                anchors {
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    left: thumbnail.right
                    verticalCenter: parent.verticalCenter
                }
                text: photoModel.count
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
            }

            onClicked: {
                pageStack.animatorPush(Qt.resolvedUrl("NextcloudAlbumsPage.qml"),
                                       { "model": nextcloudAlbums })
            }
        }
    }
}
