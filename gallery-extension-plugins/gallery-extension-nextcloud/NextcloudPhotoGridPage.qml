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
    property string albumName

    NextcloudPhotosModel {
        id: photosModel

        imageCache: NextcloudImageCache
        accountId: root.accountId
        userId: root.userId
        albumId: root.albumId
    }

    ImageGridView {
        id: imageGrid

        anchors.fill: parent
        model: photosModel

        header: PageHeader {
            title: albumName.length > 0
                   ? albumName
                     //: Heading for Nextcloud photos
                     //% "Nextcloud"
                   : qsTrId("jolla_gallery_nextcloud-la-nextcloud")
        }

        delegate: ThumbnailImage {
            id: delegateItem

            size: grid.cellSize

            onClicked: {
                var props = {
                    "imageModel": photosModel,
                    "currentIndex": model.index
                }
                pageStack.push(Qt.resolvedUrl("NextcloudFullscreenPhotoPage.qml"), props)
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

            NextcloudImageDownloader {
                id: thumbDownloader

                imageCache: NextcloudImageCache
                downloadThumbnail: true
                accountId: model.accountId
                userId: model.userId
                albumId: model.albumId
                photoId: model.photoId
            }

            Image {
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                visible: delegateItem.status === Image.Null
                source: "image://theme/icon-l-nextcloud"
            }
        }
    }
}
