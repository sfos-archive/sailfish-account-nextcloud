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

MediaSourceIcon {
    timerEnabled: false
    Image {
        anchors.fill: parent
        source: "image://theme/graphic-service-nextcloud"
        fillMode: Image.PreserveAspectCrop
        clip: true
    }
}
