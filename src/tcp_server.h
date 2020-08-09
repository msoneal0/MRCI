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
#include "unix_signal.h"

class TCPServer: public QTcpServer
{
    Q_OBJECT

private:

    QNetworkAccessManager *qNam;
    QSharedMemory         *hostSharedMem;
    QLocalServer          *controlPipe;
    QLocalSocket          *controlSocket;
    char                  *hostLoad;
    QList<QSslCertificate> sslChain;
    QSslKey                sslKey;
    QString                hostKey;
    QString                wanIP;
    quint32                maxSessions;
    quint32                flags;

    QString loadSSLData(bool onReload);
    bool    servOverloaded();
    bool    createPipe();
    void    applyPrivKey(const QString &path, QTextStream &msg);
    void    applyCerts(const QStringList &list, QTextStream &msg);
    void    incomingConnection(qintptr socketDescriptor);

private slots:

    void procPipeIn();
    void newPipeConnection();
    void closedPipeConnection();
    void sessionEnded();
    void setMaxSessions(quint32 value);
    void replyFromIpify(QNetworkReply *reply);

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

#endif // TCP_SERVER_H
