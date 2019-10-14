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
import Sailfish.Gallery 1.0
import org.nemomobile.thumbnailer 1.0
import Sailfish.TransferEngine 1.0
import Sailfish.TransferEngine.Nextcloud 1.0 // for translations

ShareDialog {
    id: root

    onAccepted: {
        shareItem.start()
    }

    SailfishShare {
        id: shareItem
        source: root.source
        serviceId: root.methodId
        userData: { "accountId": root.accountId }
        mimeType: fileInfo.mimeType
    }

    FileInfo {
        id: fileInfo
        source: root.source
    }

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentWidth: width
        contentHeight: thumbnail.height

        DialogHeader {
            //: Title for page enabling user to upload files to cloud service
            //% "Upload"
            acceptText: qsTrId("webshare-he-upload_heading")
        }

        Thumbnail {
            id: thumbnail

            mimeType: fileInfo.mimeType
            source: root.source
            sourceSize.width: Screen.width
            sourceSize.height: Screen.height / 3
            fillMode: Thumbnail.PreserveAspectCrop
            width: root.isPortrait ? Screen.width : Screen.width / 2
            height: root.isPortrait ? Screen.height / 2 : Screen.width / 2
            clip: true

            Label {
                 anchors.centerIn: parent
                 font.pixelSize: root.isPortrait ? Theme.fontSizeLarge : Theme.fontSizeMedium
                 color: Theme.secondaryColor
                 text: Format.formatFileSize(fileInfo.size)
            }
        }

        Column {
            id: settingsList
            width: root._listWidth
            spacing: Theme.paddingLarge
            anchors {
                left: root.isPortrait ? thumbnail.left : thumbnail.right
                leftMargin: root.isPortrait ? Theme.horizontalPageMargin : Theme.paddingLarge
                top: root.isPortrait ? thumbnail.bottom : thumbnail.top
                topMargin: root.isPortrait ? Theme.paddingMedium : Theme.itemSizeLarge
                right: parent.right
                rightMargin: Theme.horizontalPageMargin
            }

            Column {
                visible: fileNameLabel.text.length > 0

                Label {
                    color: Theme.highlightColor
                    //% "File"
                    text: qsTrId("webshare-la-destination_file")
                }
                Label {
                    id: fileNameLabel
                    width: settingsList.width
                    color: Theme.secondaryHighlightColor
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    text: fileInfo.fileName
                }
            }

            Column {
                visible: accountName.text.length > 0

                Label {
                    id: accountName
                    color: Theme.highlightColor
                    text: root.accountName
                }
                Label {
                    width: settingsList.width
                    color: Theme.secondaryHighlightColor
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    text: root.displayName
                }
            }

            Column {
                Label {
                    color: Theme.highlightColor
                    //: Destination folder name for Nextcloud upload
                    //% "Destination folder"
                    text: qsTrId("webshare-la-destination_folder")
                }
                Label {
                    id: targetFolderLabel
                    width: settingsList.width
                    color: Theme.secondaryHighlightColor
                    truncationMode: TruncationMode.Fade
                    font.pixelSize: Theme.fontSizeSmall
                    //: Target folder in Nextcloud. Nextcloud has a special folder called Photos
                    //: where images are uploaded. Localization should match that.
                    //% "Photos"
                    property string photosPath: qsTrId("webshare-la-nextcloud-uploads-images")

                    //: Target folder in Nextcloud. Nextcloud has a special folder called Documents
                    //: where files other than images are uploaded. Localization should match that.
                    //% "Documents"
                    property string docsPath: qsTrId("webshare-la-nextcloud-uploads-documents")

                    text: fileInfo.mimeFileType == "image" ? photosPath : docsPath
                }
            }
        }
    }
}
