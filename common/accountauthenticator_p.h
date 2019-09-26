/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_ACCOUNTAUTHENTICATOR_P_H
#define NEXTCLOUD_ACCOUNTAUTHENTICATOR_P_H

#include <QObject>

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>
#include <Accounts/AccountService>
#include <SignOn/Identity>
#include <SignOn/Error>
#include <SignOn/SessionData>
#include <SignOn/AuthSession>

class AccountAuthenticator : public QObject
{
    Q_OBJECT

public:
    AccountAuthenticator(const QString &serviceType, const QString &serviceName, QObject *parent);
    ~AccountAuthenticator();

    void signIn(int accountId);
    void setCredentialsNeedUpdate(int accountId);

Q_SIGNALS:
    void signInCompleted(const QString &serverUrl, const QString &webdavPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors);
    void signInError();

private Q_SLOTS:
    void signOnResponse(const SignOn::SessionData &response);
    void signOnError(const SignOn::Error &error);

private:
    Accounts::Manager m_manager;
    Accounts::Account *m_account = nullptr;
    SignOn::Identity *m_ident = nullptr;
    SignOn::AuthSession *m_session = nullptr;

    QString m_serviceType;
    QString m_serviceName;
    QString m_serverUrl;
    QString m_webdavPath;
    bool m_ignoreSslErrors = false;
};

#endif // NEXTCLOUD_ACCOUNTAUTHENTICATOR_P_H
