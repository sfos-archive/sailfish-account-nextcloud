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

BackgroundItem {
    id: root

    property int accountId
    property string userId
    property string albumId
    property string albumName
    property string albumThumbnailPath

    property int photoCount

    height: Theme.itemSizeExtraLarge

    Image {
        id: image
        anchors.left: parent.left
        width: Theme.itemSizeExtraLarge
        height: width
        source: albumThumbnailPath.toString().length ? albumThumbnailPath : "image://theme/icon-l-nextcloud"
        fillMode: albumThumbnailPath.toString().length ? Image.PreserveAspectCrop : Image.PreserveAspectFit
    }

    Column {
        id: column

        anchors {
            left: image.right
            leftMargin: Theme.paddingLarge
            right: parent.right
            rightMargin: Theme.paddingMedium
            verticalCenter: image.verticalCenter
        }

        Label {
            id: titleLabel
            width: parent.width
            text: albumName
            font.family: Theme.fontFamilyHeading
            font.pixelSize: Theme.fontSizeMedium
            color: highlighted ? Theme.highlightColor : Theme.primaryColor
            truncationMode: TruncationMode.Fade
        }

        Label {
            id: subtitleLabel
            width: parent.width

            //: Photos count for Nextcloud album
            //% "%n photos"
            text: qsTrId("jolla_gallery_nextcloud-album_photo_count", photoCount)
            font.family: Theme.fontFamilyHeading
            font.pixelSize: Theme.fontSizeSmall
            color: highlighted ? Theme.secondaryHighlightColor : Theme.secondaryColor
            truncationMode: TruncationMode.Fade
        }
    }
}
