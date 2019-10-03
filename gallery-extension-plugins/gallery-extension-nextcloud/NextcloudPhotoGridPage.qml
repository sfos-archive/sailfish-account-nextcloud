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

            source: model.imagePath.toString().length > 0 ? model.imagePath : model.thumbnailPath
            size: grid.cellSize

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
        }
    }
}
