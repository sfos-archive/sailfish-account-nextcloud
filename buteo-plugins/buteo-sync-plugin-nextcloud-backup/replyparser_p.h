/****************************************************************************************
**
** Copyright (c) 2019 Open Mobile Platform LLC.
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUD_BACKUP_REPLYPARSER_P_H
#define NEXTCLOUD_BACKUP_REPLYPARSER_P_H

#include <QObject>
#include <QUrl>
#include <QList>
#include <QByteArray>
#include <QDateTime>

class Syncer;
class ReplyParser
{
public:
    class FileInfo {
    public:
        QDateTime lastModified;
        QString href;
        bool isDir = false;
    };

    ReplyParser(Syncer *parent, int accountId, const QString &userId, const QUrl &serverUrl);
    ~ReplyParser();

    QList<FileInfo> parsePropFindResponse(const QByteArray &propFindResponse, const QString &remotePath);

private:
    Syncer *q;
    int m_accountId;
    QString m_userId;
    QUrl m_serverUrl;
};

Q_DECLARE_METATYPE(ReplyParser::FileInfo)

#endif // NEXTCLOUD_BACKUP_REPLYPARSER_P_H

