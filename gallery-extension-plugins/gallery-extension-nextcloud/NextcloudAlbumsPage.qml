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
import com.jolla.gallery.nextcloud 1.0

Page {
    id: albums

    property string title
    property int accountId
    property string userId

    allowedOrientations: window.allowedOrientations
    property bool _isPortrait: orientation === Orientation.Portrait
                               || orientation === Orientation.PortraitInverted

    SilicaListView {
        id: view
        anchors.fill: parent
        header: PageHeader { title: albums.title }
        cacheBuffer: screen.height
        delegate: NextcloudAlbumDelegate {
            accountId: model.accountId
            userId: model.userId
            albumId: model.albumId
            albumName: model.albumName
            albumThumbnailPath: model.thumbnailPath
            photoCount: model.photoCount
            onClicked: {
                window.pageStack.push(Qt.resolvedUrl("NextcloudPhotoGridPage.qml"),
                                      {"accountId": accountId,
                                       "userId": userId,
                                       "albumId": albumId})
            }
        }

        model: NextcloudAlbumsModel {
            imageCache: NextcloudImageCache
            // if there are more than 1 user, this page will have been pushed by the Users page,
            // with accountId+userId passed in as properties.
            accountId: view.nextcloudUsersModel.count == 1 ? view.nextcloudUsersModel.at(0).accountId : albums.accountId
            userId: view.nextcloudUsersModel.count == 1 ? view.nextcloudUsersModel.at(0).userId : albums.userId
        }

        property NextcloudUsersModel nextcloudUsersModel: NextcloudUsersModel {
            imageCache: NextcloudImageCache
        }

        VerticalScrollDecorator {}
    }
}
