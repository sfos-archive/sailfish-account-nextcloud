/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import Sailfish.Lipstick 1.0
import com.jolla.eventsview.nextcloud 1.0

Item {
    id: root

    // used by lipstick
    property var downloader
    property string providerName
    property var subviewModel
    property int animationDuration
    property bool collapsed: true
    property bool showingInActiveView
    property int eventsColumnMaxWidth
    signal expanded(int itemPosY)
    signal hasRemovableItemsChanged()
    signal mainContentHeightChanged()

    property int _expansionThreshold: 5
    property int _expansionMaximum: 10
    property bool _manuallyExpanded

    width: parent.width
    height: listView.model.count === 0 ? 0 : expansionToggle.y + expansionToggle.height

    onCollapsedChanged: {
        if (!collapsed) {
            root._manuallyExpanded = false
        }
    }

    NotificationGroupHeader {
        id: headerItem

        name: root.providerName
        indicator.iconSource: "image://theme/graphic-service-nextcloud"
        memberCount: totalItemCount
        totalItemCount: listView.model.count
        userRemovable: false
    }

    ListView {
        id: listView
        anchors.top: headerItem.bottom
        width: parent.width
        height: Screen.height * 1000 // Ensures the view is fully populated without needing to bind height: contentHeight

        interactive: false
        model: boundedModel
    }

    NotificationExpansionButton {
        id: expansionToggle

        y: headerItem.height + listView.contentHeight
        expandable: listView.model.count > _expansionThreshold

        onClicked: {
            if (!root._manuallyExpanded) {
                var itemPosY = listView.contentHeight + headerItem.height - Theme.paddingLarge
                root._manuallyExpanded = true
                root.expanded(itemPosY)
            } else {
                root.expandedClicked()
            }
        }
    }

    NextcloudEventCache {
        id: evCache
    }

    NextcloudEventsModel {
        id: eventModel

        eventCache: evCache

        Component.onCompleted: {
            // Use the first found account, which is the one most recently added.
            var ids = accountManager.providerAccountIdentifiers(root.providerName)
            if (ids.length > 0) {
                accountId = ids[0]
            }
        }
    }

    BoundedModel {
        id: boundedModel
        model: eventModel
        maximumCount: root._manuallyExpanded ? root._expansionMaximum : root._expansionThreshold

        delegate: NextcloudFeedItem {
            id: delegateItem

            subject: model.eventText
            icon.source: imageDownloader.imagePath != ""
                         ? imageDownloader.imagePath
                         : "image://theme/icon-l-nextcloud"
            timestamp: model.timestamp
            eventUrl: model.eventUrl

            NextcloudEventImageDownloader {
                id: imageDownloader
                accountId: eventModel.accountId
                eventCache: evCache
                eventId: model.eventId
            }
        }
    }

    AccountManager {
        id: accountManager
    }
}
