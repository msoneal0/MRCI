#ifndef SOCKET_H
#define SOCKET_H

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

#include "common.h"
#include "int_loader.h"
#include "make_cert.h"
#include "cmd_executor.h"

class Session : public QObject
{
    Q_OBJECT

private:

    QList<QByteArray> p2pAccepted;
    QList<QByteArray> p2pPending;
    QList<QString>    chList;
    QSharedMemory    *exeDebugInfo;
    QLocalServer     *ipcServ;
    QLocalSocket     *ipcLink;
    QSslSocket       *tcpSocket;
    QProcess         *slaveProc;
    RWSharedObjs     *rwShared;
    SharedObjs       *shared;
    CmdExecutor      *executor;
    QTime             lastExeCrash;
    QString           peerIp;
    QString           pipeName;
    QString           userName;
    QString           groupName;
    QString           displayName;
    QString           appName;
    ushort            clientMajor;
    ushort            clientMinor;
    ushort            clientPatch;
    QByteArray        chIds;
    QByteArray        wrAbleChIds;
    QByteArray        sessionId;
    QByteArray        userId;
    uint              hostRank;
    uint              flags;
    uint              ipcFrameSize;
    uint              tcpFrameSize;
    uchar             ipcFrameType;
    uchar             tcpFrameType;
    quint16           ipcFrameCmdId;
    quint16           tcpFrameCmdId;
    bool              activeUpdate;
    bool              chOwnerOverride;
    int               exeCrashCount;

    void genSessionId();
    void castPeerInfo();
    void sendLocalInfo();
    void rdExeDebug();
    void addIpAction(const QString &action);
    void modLoadCrashCheck(const QString &crashInfo);

private slots:

    void logout();
    void newIPCLink();
    void ipcConnected();
    void ipcDisconnected();
    void dataFromClient();
    void dataFromIPC();
    void payloadDeleted();
    void sendStdout();
    void sendStderr();
    void exeStarted();
    void closeInstance();
    void authOk();
    void ipcOk();
    void newIPCTimeout();
    void exeFinished(int ret, QProcess::ExitStatus status);
    void exeError(QProcess::ProcessError err);
    void ipcError(QLocalSocket::LocalSocketError socketError);
    void dataToClient(quint16 cmdId, const QByteArray &data, uchar typeId);

public:

    explicit Session(QObject *parent = nullptr);

    void initAsMain(QSslSocket *tcp);
    void startAsSlave(const QStringList &args);
    bool isSlave();
    bool isMain();

public slots:

    void backendDataIn(quint16 cmdId, const QByteArray &data);
    void peersDataIn(quint16 cmdId, const QByteArray &data);
    void connectToPeer(const QSharedPointer<SessionCarrier> &peer);
    void endSession();
    void run();

signals:

    void dataToCommand(quint16 cmdId, const QByteArray &data, uchar dType);
    void backendToPeers(quint16 cmdId, const QByteArray data);
    void connectPeers(QSharedPointer<SessionCarrier> peer);
    void setMaxSessions(uint value);
    void unloadModFile(const QString &modName);
    void loadModFile(const QString &modName);
    void delayedModDel(const QString &modName);
    void ended();
    void closeExe();
    void closeServer();
    void resServer();
    void loadCommands();
};

#endif // SOCKET_H
