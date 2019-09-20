/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_SHARING_AUTH_P_H
#define NEXTCLOUD_SHARING_AUTH_P_H

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QVector>
#include <QtCore/QString>

#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>
#include <Accounts/AccountService>
#include <SignOn/Identity>
#include <SignOn/Error>
#include <SignOn/SessionData>
#include <SignOn/AuthSession>

class Auth : public QObject
{
    Q_OBJECT

public:
    Auth(QObject *parent);
    ~Auth();

    void signIn(int accountId);
    void setCredentialsNeedUpdate(int accountId);

Q_SIGNALS:
    void signInCompleted(int accountId, const QString &serverUrl, const QString &webdavPath, const QString &username, const QString &password, const QString &accessToken, bool ignoreSslErrors);
    void signInError(int accountId);

private Q_SLOTS:
    void signOnResponse(const SignOn::SessionData &response);
    void signOnError(const SignOn::Error &error);

protected:
    Accounts::Manager *manager() { return &m_manager; }
    QString m_serviceName;

private:
    Accounts::Manager m_manager;
    struct AuthData {
        int accountId;
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

#endif // NEXTCLOUD_SHARING_AUTH_P_H
