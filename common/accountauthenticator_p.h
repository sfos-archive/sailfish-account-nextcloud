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

#include <QtCore/QObject>
#include <QtCore/QVector>

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
    AccountAuthenticator(QObject *parent);
    ~AccountAuthenticator();

    void signIn(int accountId, const QString &serviceName);
    void setCredentialsNeedUpdate(int accountId, const QString &serviceName);

Q_SIGNALS:
    void signInCompleted(int accountId, const QString &serviceName,
                         const QString &serverUrl, const QString &webdavPath,
                         const QString &username, const QString &password,
                         const QString &accessToken, bool ignoreSslErrors);
    void signInError(int accountId, const QString &serviceName);

private Q_SLOTS:
    void signOnResponse(const SignOn::SessionData &response);
    void signOnError(const SignOn::Error &error);

protected:
    Accounts::Manager *manager() { return &m_manager; }

private:
    Accounts::Manager m_manager;
    struct AuthData {
        int accountId;
        QString serviceName;
        QString mechanism;
        QVariantMap signonSessionData;
        Accounts::Account *account;
        SignOn::Identity *identity;
        SignOn::AuthSession *authSession;
        QString serverAddress;
        QString webdavPath;
        bool ignoreSslErrors;
    };
    QVector<AuthData> m_authData;
};

#endif // NEXTCLOUD_ACCOUNTAUTHENTICATOR_P_H
