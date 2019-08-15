/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudshareservicestatus.h"

#include <Accounts/Account>
#include <SignOn/SessionData>
#include <QtDebug>

#define NEXTCLOUD_PROVIDER_NAME    "nextcloud"
#define NEXTCLOUD_SERVICE_NAME     "nextcloud-sharing"

NextcloudShareServiceStatus::NextcloudShareServiceStatus(QObject *parent)
    : QObject(parent)
{
}

bool NextcloudShareServiceStatus::signInParameters(QVariantMap *params) const
{
    if (!params) {
        qWarning() << Q_FUNC_INFO << "NULL SignInParams object!";
        return false;
    }

    QString clientId = storedKeyValue(NEXTCLOUD_PROVIDER_NAME, NEXTCLOUD_SERVICE_NAME, "client_id");
    QString clientSecret = storedKeyValue(NEXTCLOUD_PROVIDER_NAME, NEXTCLOUD_SERVICE_NAME, "client_secret");
    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "No valid OAuth2 keys found";
        return false;
    }

    // Set required parameters for the Nextcloud
    params->insert("ClientId", clientId);
    params->insert("ClientSecret", clientSecret);
    params->insert("UiPolicy", SignOn::NoUserInteractionPolicy);
    return true;
}

bool NextcloudShareServiceStatus::signInResponseHandler(const QVariantMap &receivedData, QVariantMap &outputData)
{
    QLatin1String accessToken("AccessToken");
    if (!receivedData.contains(accessToken)) {
        return false;
    }

    Accounts::Account *account = receivedData.value("account").value<Accounts::Account*>();
    if (!account) {
        qWarning() << Q_FUNC_INFO << "Empty account";
        return false;
    }

    QString host = account->value(QStringLiteral("api/Host")).toString();
    if (host.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Missing response parameters";
        return false;
    }

    QString contentApiUrl = account->value(QStringLiteral("api/Host")).toString();

    outputData.insert(QStringLiteral("host"), host);
    outputData.insert(accessToken, receivedData[accessToken]);
    outputData.insert(QStringLiteral("contentApiUrl"), contentApiUrl);

    return true;
}
