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
import Sailfish.Accounts 1.0
import com.jolla.eventsview.nextcloud 1.0

Column {
    id: root

    // used by lipstick
    property var downloader
    property string providerName
    property var subviewModel
    property int animationDuration
    property bool collapsed: true
    property bool showingInActiveView
    property int eventsColumnMaxWidth
    property bool userRemovable
    property bool hasRemovableItems
    property real mainContentHeight

    signal expanded(int itemPosY)

    // used by lipstick
    function findMatchingRemovableItems(filterFunc, matchingResults) {
        for (var i = 0; i < accountFeedRepeater.count; ++i) {
            accountFeedRepeater.itemAt(i).findMatchingRemovableItems(filterFunc, matchingResults)
        }
    }

    // used by lipstick
    function removeAllNotifications() {
        if (userRemovable) {
            for (var i = 0; i < accountFeedRepeater.count; ++i) {
                accountFeedRepeater.itemAt(i).removeAllNotifications()
            }
        }
    }

    width: parent.width

    AccountManager {
        id: accountManager

        Component.onCompleted: {
            var accountIds = accountManager.providerAccountIdentifiers(root.providerName)
            var model = []
            for (var i = 0; i < accountIds.length; ++i) {
                model.push(accountIds[i])
            }
            accountFeedRepeater.model = model
        }
    }

    Repeater {
        id: accountFeedRepeater

        delegate: NextcloudFeed {
            width: root.width

            accountId: modelData
            collapsed: root.collapsed
            showingInActiveView: root.showingInActiveView

            onExpanded: {
                root.collapsed = false
                root.expanded(itemPosY)
            }

            onUserRemovableChanged: {
                var _userRemovable = true
                for (var i = 0; i < accountFeedRepeater.count; ++i) {
                    var item = accountFeedRepeater.itemAt(i)
                    if (!!item && !item.userRemovable) {
                        _userRemovable = false
                        break
                    }
                }
                root.userRemovable = _userRemovable
            }

            onHasRemovableItemsChanged: {
                var _hasRemovableItems = true
                for (var i = 0; i < accountFeedRepeater.count; ++i) {
                    var item = accountFeedRepeater.itemAt(i)
                    if (!!item && !item.hasRemovableItems) {
                        _hasRemovableItems = false
                        break
                    }
                }
                root.hasRemovableItems = _hasRemovableItems
            }

            onMainContentHeightChanged: {
                var _mainContentHeight = 0
                for (var i = 0; i < accountFeedRepeater.count; ++i) {
                    var item = accountFeedRepeater.itemAt(i)
                    if (!!item) {
                        _mainContentHeight += item.mainContentHeight
                    }
                }
                root.mainContentHeight = _mainContentHeight
            }
        }
    }
}
