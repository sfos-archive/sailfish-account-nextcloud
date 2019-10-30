import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Gallery 1.0
import com.jolla.gallery 1.0
import com.jolla.gallery.nextcloud 1.0

Page {
    id: root

    property int accountId
    property string userId
    property string albumId

    property NextcloudPhotosModel photosModel: NextcloudPhotosModel {
        imageCache: NextcloudImageCache
        accountId: root.accountId
        userId: root.userId
        albumId: root.albumId
    }

    ImageGridView {
        id: imageGrid
        anchors.fill: parent

        model: photosModel
        delegate: ThumbnailImage {
            id: delegateItem

            property int accountId: model.accountId
            property string userId: model.userId
            property string albumId: model.albumId
            property string photoId: model.photoId

            size: grid.cellSize

            property NextcloudImageDownloader thumbDownloader: NextcloudImageDownloader {
                imageCache: NextcloudImageCache
                downloadThumbnail: true
                accountId: delegateItem.accountId
                userId: delegateItem.userId
                albumId: delegateItem.albumId
                photoId: delegateItem.photoId
            }

            Image {
                id: placeholderItem
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                visible: delegateItem.source.toString().length === 0
                source: "image://theme/icon-l-nextcloud"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    window.pageStack.push(Qt.resolvedUrl("NextcloudFullscreenPhotoPage.qml"),
                                          {"accountId": delegateItem.accountId,
                                           "userId": delegateItem.userId,
                                           "albumId": delegateItem.albumId,
                                           "photoId": delegateItem.photoId})
                }
            }

            Component.onCompleted: {
                // do this manually rather than binding, to avoid flicker,
                // as the model.thumbnailPath will be updated due to the
                // thumbnail being downloaded, so avoid capturing it.
                delegateItem.source = model.thumbnailPath.toString().length > 0
                                    ? model.thumbnailPath
                                    : model.imagePath.toString().length > 0
                                        ? model.imagePath
                                        : Qt.binding(function() { return thumbDownloader.imagePath })
            }
        }
    }
}
