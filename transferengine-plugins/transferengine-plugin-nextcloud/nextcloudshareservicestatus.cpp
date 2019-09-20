/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#include "nextcloudshareservicestatus.h"

#include <QtDebug>

NextcloudShareServiceStatus::NextcloudShareServiceStatus(QObject *parent)
    : Auth(parent)
{
    connect(this, &Auth::signInCompleted, this, &NextcloudShareServiceStatus::signInResponseHandler);
    connect(this, &Auth::signInError, this, &NextcloudShareServiceStatus::signInErrorHandler);
}

void NextcloudShareServiceStatus::signInResponseHandler(int accountId, const QString &serverAddress, const QString &webdavPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors)
{
    if (!m_accountIdToDetailsIdx.contains(accountId)) {
        return;
    }

    AccountDetails &accountDetails(m_accountDetails[m_accountIdToDetailsIdx[accountId]]);
    accountDetails.accessToken = accessToken;
    accountDetails.username = username;
    accountDetails.password = password;
    accountDetails.serverAddress = serverAddress;
    accountDetails.webdavPath = webdavPath.isEmpty() ? QStringLiteral("/remote.php/webdav/") : webdavPath;
    accountDetails.photosPath = accountDetails.webdavPath.endsWith("/")
                              ? QStringLiteral("%1Photos/").arg(accountDetails.webdavPath)
                              : QStringLiteral("%1/Photos/").arg(accountDetails.webdavPath);
    accountDetails.documentsPath = accountDetails.webdavPath.endsWith("/")
                              ? QStringLiteral("%1Documents/").arg(accountDetails.webdavPath)
                              : QStringLiteral("%1/Documents/").arg(accountDetails.webdavPath);
    accountDetails.ignoreSslErrors = ignoreSslErrors;

    setAccountDetailsState(accountId, Populated);
}

void NextcloudShareServiceStatus::signInErrorHandler(int accountId)
{
    setAccountDetailsState(accountId, Error);
}

void NextcloudShareServiceStatus::setAccountDetailsState(int accountId, AccountDetailsState state)
{
    if (!m_accountIdToDetailsIdx.contains(accountId)) {
        return;
    }

    m_accountDetailsState[accountId] = state;

    bool anyWaiting = false;
    bool anyPopulated = false;
    Q_FOREACH (const int accountId, m_accountDetailsState.keys()) {
        AccountDetailsState accState = m_accountDetailsState.value(accountId, NextcloudShareServiceStatus::Waiting);
        if (accState == NextcloudShareServiceStatus::Waiting) {
            anyWaiting = true;
        } else if (accState == NextcloudShareServiceStatus::Populated) {
            anyPopulated = true;
        }
    }

    if (!anyWaiting) {
        if (anyPopulated) {
            emit serviceReady();
        } else {
            emit serviceError(QStringLiteral("Unable to retrieve Nextcloud account credentials"));
        }
    }
}

int NextcloudShareServiceStatus::count() const
{
    return m_accountDetails.count();
}

void NextcloudShareServiceStatus::queryStatus(QueryStatusMode mode)
{
    m_accountDetails.clear();
    m_accountIdToDetailsIdx.clear();
    m_accountDetailsState.clear();

    bool signInActive = false;
    foreach(uint id, manager()->accountList()) {
        Accounts::Account *acc = manager()->account(id);

        if (!acc) {
            qWarning() << Q_FUNC_INFO << "Failed to get account for id: " << id;
            continue;
        }

        acc->selectService(Accounts::Service());
        const Accounts::Service service(manager()->service(m_serviceName));
        const Accounts::ServiceList services = acc->services();
        bool found = false;
        Q_FOREACH (const Accounts::Service &s, services) {
            if (s.name() == m_serviceName) {
                found = true;
                break;
            }
        }

        if (acc->enabled() && service.isValid() && found) {
            if (acc->value(QStringLiteral("CredentialsNeedUpdate")).toBool() == true) {
                // credentials need update for global service, skip the account
                qWarning() << Q_FUNC_INFO << "Credentials need update for account id: " << id;
                continue;
            }
            acc->selectService(service);
            if (acc->value(QStringLiteral("CredentialsNeedUpdate")).toBool() == true) {
                // credentials need update for sharing service, skip the account
                qWarning() << Q_FUNC_INFO << "Credentials need update for account id: " << id;
                acc->selectService(Accounts::Service());
                continue;
            }

            if (acc->enabled()) {
                if (!m_accountIdToDetailsIdx.contains(id)) {
                    AccountDetails details;
                    details.accountId = id;
                    details.providerName = manager()->provider(acc->providerName()).displayName();
                    details.serviceName = manager()->service(m_serviceName).displayName();
                    details.displayName = acc->displayName();
                    m_accountIdToDetailsIdx.insert(id, m_accountDetails.size());
                    m_accountDetails.append(details);
                }

                if (mode == NextcloudShareServiceStatus::SignInMode) {
                    signInActive = true;
                    signIn(id);
                }
            }
            acc->selectService(Accounts::Service());
        }
    }

    // either no enabled accounts, or mode was passive (no sign in required).
    if (!signInActive) {
        emit serviceReady();
    }
}

NextcloudShareServiceStatus::AccountDetails NextcloudShareServiceStatus::details(int index) const
{
    if (index < 0 || m_accountDetails.size() <= index) {
        qWarning() << Q_FUNC_INFO << "Index out of range";
        return AccountDetails();
    }

    return m_accountDetails.at(index);
}

NextcloudShareServiceStatus::AccountDetails NextcloudShareServiceStatus::detailsByIdentifier(int accountIdentifier) const
{
    if (!m_accountIdToDetailsIdx.contains(accountIdentifier)) {
        qWarning() << Q_FUNC_INFO << "No details known for account with identifier" << accountIdentifier;
        return AccountDetails();
    }

    return m_accountDetails[m_accountIdToDetailsIdx[accountIdentifier]];
}
