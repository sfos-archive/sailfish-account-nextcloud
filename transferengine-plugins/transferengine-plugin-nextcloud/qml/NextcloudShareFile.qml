/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/
import QtQuick 2.6
import Sailfish.Silica 1.0
import Sailfish.TransferEngine 1.0
import Sailfish.TransferEngine.Nextcloud 1.0 // for translations
import Nemo.Configuration 1.0

ShareFilePreviewDialog {
    id: root

    imageScaleVisible: false
    descriptionVisible: false
    metaDataSwitchVisible: false

    remoteDirName: {
        switch (fileInfo.mimeFileType) {
        case "image":
            return pathConfig.images
        default:
            return pathConfig.documents
        }
    }
    remoteDirReadOnly: false

    onRemoteDirNameChanged: {
        switch (fileInfo.mimeFileType) {
        case "image":
            pathConfig.images = remoteDirName
            break
        default:
            pathConfig.documents = remoteDirName
            break
        }
    }

    onAccepted: {
        shareItem.start()
    }

    ConfigurationGroup {
        id: pathConfig

        property string images: "Photos"
        property string documents: "Documents"

        path: "/sailfish/nemo-transferengine/plugins/nextcloud"
    }
}
