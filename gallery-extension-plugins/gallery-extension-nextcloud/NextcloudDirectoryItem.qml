/****************************************************************************************
** Copyright (c) 2019 Open Mobile Platform LLC.
** Copyright (c) 2023 Jolla Ltd.
**
** All rights reserved.
**
** This file is part of Sailfish Nextcloud account package.
**
** You may use this file under the terms of BSD license as follows:
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this
**    list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice,
**    this list of conditions and the following disclaimer in the documentation
**    and/or other materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
** SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
** CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
** OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************************/

import QtQuick 2.6
import Sailfish.Silica 1.0

Item {
    id: root

    property alias title: titleLabel.text
    property alias titleLabel: titleLabel
    property alias countText: countLabel.text
    property alias icon: icon

    width: parent.width
    height: Theme.itemSizeExtraLarge

    Label {
        id: titleLabel

        anchors {
            left: parent.left
            leftMargin: Theme.paddingLarge
            right: iconContainer.left
            rightMargin: Theme.paddingLarge
            top: parent.top
            bottom: parent.bottom
        }
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WrapAnywhere
        font.pixelSize: Theme.fontSizeLarge
        fontSizeMode: Text.VerticalFit
    }

    Rectangle {
        id: iconContainer

        anchors.left: parent.horizontalCenter
        width: Theme.itemSizeExtraLarge
        height: Theme.itemSizeExtraLarge
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.rgba(Theme.primaryColor, 0.1) }
            GradientStop { position: 1.0; color: "transparent" }
        }

        HighlightImage {
            id: icon

            anchors.centerIn: parent
            source: "image://theme/icon-m-file-folder"
        }
    }

    Label {
        id: countLabel

        anchors {
            right: parent.right
            leftMargin: Theme.horizontalPageMargin
            left: iconContainer.right
            verticalCenter: parent.verticalCenter
        }
        font.pixelSize: Theme.fontSizeLarge
    }
}
