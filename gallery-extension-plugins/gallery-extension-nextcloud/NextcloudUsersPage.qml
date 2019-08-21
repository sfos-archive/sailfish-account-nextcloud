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

    SilicaListView {
        id: view
        anchors.fill: parent
        header: PageHeader {}
        model: NextcloudUsersModel {
            id: nextcloudUsers
            imageCache: NextcloudImageCache
        }

        delegate: BackgroundItem {
            id: delegateItem

            property int accountId: model.accountId
            property string userId: model.userId
            property int userCount: nextcloudUsers.count

            width: parent.width
            height: Math.max(thumbnail.height, titleLabel.height, countLabel.height)

            NextcloudPhotosModel {
                id: photosModel
                imageCache: NextcloudImageCache
                accountId: delegateItem.accountId
                userId: delegateItem.userId
                // TODO: getThumbnailForRow()
            }

            Label {
                id: titleLabel
                elide: Text.ElideRight
                font.pixelSize: Theme.fontSizeLarge
                text: model.userId
                color: delegateItem.down ? Theme.highlightColor : Theme.primaryColor
                anchors {
                    right: thumbnail.left
                    rightMargin: Theme.paddingLarge
                    verticalCenter: parent.verticalCenter
                }
            }

            Image {
                id: thumbnail
                anchors.left: parent.horizontalCenter
                opacity: delegateItem.down ? Theme.opacityHigh : 1
                source: /* photosModel.count > 0 ? photosModel.getThumbnailForRow(0) : */ "image://theme/graphic-service-nextcloud"
                width: Theme.itemSizeExtraLarge
                height: width
            }

            Label {
                id: countLabel
                anchors {
                    right: parent.right
                    leftMargin: Theme.horizontalPageMargin
                    left: thumbnail.right
                    verticalCenter: parent.verticalCenter
                }
                text: photosModel.count
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
            }

            onClicked: {
                window.pageStack.push(Qt.resolvedUrl("NextcloudAlbumsPage.qml"),
                                      {"accountId": delegateItem.accountId,
                                       "userId": delegateItem.userId})
            }
        }
    }
}
