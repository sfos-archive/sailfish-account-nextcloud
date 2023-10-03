/****************************************************************************************
** Copyright (c) 2019 - 2020 Open Mobile Platform LLC.
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

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Lipstick 1.0
import com.jolla.eventsview.nextcloud 1.0

NotificationGroupMember {
    id: root

    property alias icon: image
    property alias subject: subjectLabel.text
    property alias message: messageLabel.text
    property var timestamp
    property string eventUrl

    width: parent.width
    contentHeight: Math.max(image.y + image.height, content.y + content.height) + Theme.paddingLarge

    onTriggered: {
        if (eventUrl.length > 0) {
            Qt.openUrlExternally(eventUrl)
        }
    }

    Image {
        id: image
        y: Theme.paddingLarge
        width: Theme.iconSizeMedium
        height: Theme.iconSizeMedium
    }

    Column {
        id: content

        anchors {
            left: image.right
            leftMargin: Theme.paddingMedium
            top: image.top
            topMargin: -Theme.paddingSmall
        }

        width: root.width - x - root.contentLeftMargin - Theme.paddingMedium
        spacing: Theme.paddingSmall

        Label {
            id: subjectLabel
            width: parent.width
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            font.bold: true
        }

        Label {
            id: messageLabel
            visible: text.length !== 0
            width: parent.width
            elide: Text.ElideRight
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
        }

        Label {
            id: timestampLabel
            text: Format.formatDate(root.timestamp, Format.TimeElapsed)
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
        }
    }
}
