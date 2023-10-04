/****************************************************************************************
** Copyright (c) 2013 - 2023 Jolla Ltd.
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

#ifndef NEXTCLOUD_PROCESSMUTEX_P_H
#define NEXTCLOUD_PROCESSMUTEX_P_H

#include <QString>
#include <QMutex>

namespace SyncCache {

class Semaphore
{
public:
    Semaphore(const char *identifier, int initial);
    Semaphore(const char *identifier, size_t count, const int *initialValues);
    ~Semaphore();

    bool isValid() const;

    bool decrement(size_t index = 0, bool wait = true, size_t timeoutMs = 0);
    bool increment(size_t index = 0, bool wait = true, size_t timeoutMs = 0);

    int value(size_t index = 0) const;

private:
    void error(const char *msg, int error);

    QString m_identifier;
    int m_id;
};

class ProcessMutex
{
    Semaphore m_semaphore;
    QMutex m_mutex;
    bool m_initialProcess;

public:
    ProcessMutex(const QString &path);
    bool lock();
    bool unlock();
    bool isLocked() const;
    bool isInitialProcess() const;
};

} // namespace SyncCache

#endif // NEXTCLOUD_PROCESSMUTEX_P_H
