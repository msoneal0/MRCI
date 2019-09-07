#ifndef TCP_SERVER_H
#define TCP_SERVER_H

//    This file is part of MRCI.

//    MRCI is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    MRCI is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with MRCI under the LICENSE.md file. If not, see
//    <http://www.gnu.org/licenses/>.

#include "db.h"
#include "common.h"
#include "session.h"
#include "make_cert.h"
#include "openssl/ssl.h"

class TCPServer: public QTcpServer
{
    Q_OBJECT

private:

    QSharedMemory *sessionCounter;
    QLocalServer  *controlPipe;
    QLocalSocket  *controlSocket;
    QTimer        *dirDelTimer;
    QStringList    dirsToDel;
    uint           flags;

    bool servOverloaded();
    bool inBanList(const QString &ip);
    void syncModPath();
    void incomingConnection(qintptr socketDescriptor);

private slots:

    void delDir();
    void procPipeIn();
    void newPipeConnection();
    void closedPipeConnection();
    void sessionEnded();
    void delayedDirDel(const QString &path);

public slots:

    void resServer();
    void closeServer();

public:

    explicit TCPServer(QObject *parent = nullptr);

    bool start();

signals:

    void connectPeers(QSharedPointer<SessionCarrier> peer);
    void endAllSessions();
};

//--------------------------------

class IPCServer: public QLocalServer
{
    Q_OBJECT

private:

    uint flags;

    void incomingConnection(qintptr socketDescriptor);

public:

    explicit IPCServer(QObject *parent = nullptr);

    bool start();
};

#endif // TCP_SERVER_H
