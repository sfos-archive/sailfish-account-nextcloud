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

import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Accounts 1.0
import com.jolla.settings.accounts 1.0

OnlineSyncAccountCreationAgent {
    provider: accountProvider
    services: [
        accountManager.service("nextcloud-backup"),
        accountManager.service("nextcloud-caldav"),
        accountManager.service("nextcloud-carddav"),
        accountManager.service("nextcloud-images"),
        accountManager.service("nextcloud-posts"),
        accountManager.service("nextcloud-sharing")
    ]

    sharedScheduleServices: [
        accountManager.service("nextcloud-carddav"),
        accountManager.service("nextcloud-caldav"),
        accountManager.service("nextcloud-images"),
        accountManager.service("nextcloud-posts"),
    ]

    webdavPath: AccountsUtil.joinServerPathInAddress(serverAddress, "/remote.php/dav/files/" + username)
    imagesPath: AccountsUtil.joinServerPathInAddress(serverAddress, "/remote.php/dav/files/" + username + "/Photos")
    backupsPath: AccountsUtil.joinServerPathInAddress(serverAddress, "/remote.php/dav/files/" + username + "/Sailfish OS/Backups")

    showAdvancedSettings: true
}
