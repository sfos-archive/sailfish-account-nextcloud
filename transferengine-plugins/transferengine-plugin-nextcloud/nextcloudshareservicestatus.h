/****************************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** All rights reserved.
**
** License: Proprietary.
**
****************************************************************************************/

#ifndef NEXTCLOUDSHARESERVICESTATUS_H
#define NEXTCLOUDSHARESERVICESTATUS_H

#include <QtCore/QObject>

class NextcloudShareServiceStatus : public QObject
{
    Q_OBJECT

public:
    NextcloudShareServiceStatus(QObject *parent = 0);

protected:
    bool signInParameters(QVariantMap *params) const;
    bool signInResponseHandler(const QVariantMap &receivedData, QVariantMap &outputData);
};

#endif // NEXTCLOUDSHARESERVICESTATUS_H
