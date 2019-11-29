/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
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
    property bool hasRemovableItems: accountFeedRepeater.count > 0
    signal expanded(int itemPosY)
    signal mainContentHeightChanged()

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

            onExpanded: root.expanded(itemPosY)
        }
    }
}
