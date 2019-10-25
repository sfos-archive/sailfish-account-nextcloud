/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_EVENTSVIEW_EVENTMODEL_H
#define NEXTCLOUD_EVENTSVIEW_EVENTMODEL_H

#include "eventcache.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkRequest>
#include <QtQml/QQmlParserStatus>

// accounts&sso
#include <Accounts/Account>
#include <Accounts/Manager>
#include <Accounts/Service>
#include <Accounts/AccountService>
#include <SignOn/Identity>
#include <SignOn/Error>
#include <SignOn/SessionData>
#include <SignOn/AuthSession>

class NextcloudEventCache : public SyncCache::EventCache
{
    Q_OBJECT

public:
    explicit NextcloudEventCache(QObject *parent = Q_NULLPTR);

    void openDatabase(const QString &) Q_DECL_OVERRIDE;
    void populateEventImage(int idempToken, int accountId, const QString &eventId, const QNetworkRequest &) Q_DECL_OVERRIDE;

    enum PendingRequestType {
        PopulateEventImageType
    };

    struct PendingRequest {
        PendingRequestType type;
        int idempToken;
        int accountId;
        QString eventId;
    };

private Q_SLOTS:
    void performRequests();
    void signOnResponse(const SignOn::SessionData &response);
    void signOnError(const SignOn::Error &error);

private:
    QList<PendingRequest> m_pendingRequests;
    QList<int> m_pendingAccountRequests;
    QHash<int, Accounts::Account*> m_accounts;
    QHash<int, SignOn::Identity*> m_identities;
    QHash<int, int> m_signOnFailCount;
    QHash<int, QPair<QString, QString> > m_accountIdCredentials;
    QHash<int, QString> m_accountIdAccessTokens;
    Accounts::Manager m_manager;

    void signIn(int accountId);
    void cleanUpAccount(int accountId, SignOn::AuthSession *session = Q_NULLPTR);
    void performRequest(const PendingRequest &request);
    QNetworkRequest templateRequest(int accountId) const;
};
Q_DECLARE_METATYPE(NextcloudEventCache::PendingRequest)
Q_DECLARE_TYPEINFO(NextcloudEventCache::PendingRequest, Q_MOVABLE_TYPE);

class NextcloudEventsModel : public QAbstractListModel, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::EventCache* eventCache READ eventCache WRITE setEventCache NOTIFY eventCacheChanged)
    Q_PROPERTY(int accountId READ accountId WRITE setAccountId NOTIFY accountIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowCountChanged)

public:
    explicit NextcloudEventsModel(QObject *parent = Q_NULLPTR);

    // QQmlParserStatus
    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

    // QQmlAbstractListModel
    QModelIndex index(int row, int column, const QModelIndex &parent) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;

    enum Roles {
        AccountIdRole = Qt::UserRole + 1,
        EventIdRole,
        EventSubjectRole,
        EventTextRole,
        EventUrlRole,
        ImageUrlRole,
        ImagePathRole,
        TimestampRole,
    };
    Q_ENUM(Roles)

    SyncCache::EventCache* eventCache() const;
    void setEventCache(SyncCache::EventCache *cache);

    int accountId() const;
    void setAccountId(int id);

    Q_INVOKABLE QVariantMap at(int row) const;
    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void eventCacheChanged();
    void accountIdChanged();
    void rowCountChanged();

private:
    void loadData();
    bool m_deferLoad;
    SyncCache::EventCache* m_eventCache;
    int m_accountId;
    QVector<SyncCache::Event> m_data;
};

class NextcloudEventImageDownloader : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(SyncCache::EventCache* eventCache READ eventCache WRITE setEventCache NOTIFY eventCacheChanged)
    Q_PROPERTY(int accountId READ accountId WRITE setAccountId NOTIFY accountIdChanged)
    Q_PROPERTY(QString eventId READ eventId WRITE setEventId NOTIFY eventIdChanged)
    Q_PROPERTY(QUrl imagePath READ imagePath NOTIFY imagePathChanged)

public:
    explicit NextcloudEventImageDownloader(QObject *parent = Q_NULLPTR);

    // QQmlParserStatus
    void classBegin() Q_DECL_OVERRIDE;
    void componentComplete() Q_DECL_OVERRIDE;

    SyncCache::EventCache* eventCache() const;
    void setEventCache(SyncCache::EventCache *cache);

    int accountId() const;
    void setAccountId(int id);

    QString eventId() const;
    void setEventId(const QString &eventId);

    QUrl imagePath() const;

Q_SIGNALS:
    void eventCacheChanged();
    void accountIdChanged();
    void eventIdChanged();
    void imagePathChanged();

private:
    void loadImage();
    bool m_deferLoad;
    SyncCache::EventCache* m_eventCache;
    int m_accountId;
    QString m_eventId;
    QUrl m_imagePath;
    int m_idempToken;
};

#endif // NEXTCLOUD_EVENTSVIEW_EVENTMODEL_H
