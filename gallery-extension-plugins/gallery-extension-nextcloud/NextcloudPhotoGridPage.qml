import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Gallery 1.0
import com.jolla.gallery 1.0
import com.jolla.gallery.nextcloud 1.0
import org.nemomobile.thumbnailer 1.0

Page {
    id: root

    property int accountId
    property string userId
    property string albumId
    property string albumName

    NextcloudPhotoModel {
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

            source: thumbDownloader.status === NextcloudImageDownloader.Ready
                    ? thumbDownloader.imagePath
                    : model.imagePath

            onClicked: {
                var props = {
                    "imageModel": photosModel,
                    "currentIndex": model.index
                }
                pageStack.push(Qt.resolvedUrl("NextcloudFullscreenPhotoPage.qml"), props)
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

            HighlightImage {
                anchors.fill: parent
                source: "image://theme/icon-l-nextcloud"
                highlighted: delegateItem.containsMouse
                opacity: highlighted ? Theme.opacityHigh : 1
                visible: delegateItem.status !== Thumbnail.Ready
            }
        }
    }
}
