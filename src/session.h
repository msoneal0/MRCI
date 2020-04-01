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
#include "module.h"
#include "make_cert.h"
#include "cmd_proc.h"

QByteArray wrFrame(quint32 cmdId, const QByteArray &data, uchar dType);

class Session : public MemShare
{
    Q_OBJECT

private:

    QSslSocket                        *tcpSocket;
    QString                            currentDir;
    QHash<QString, QStringList>        modCmdNames;
    QHash<quint32, QList<QByteArray> > frameQueue;
    QHash<quint32, CmdProcess*>        cmdProcesses;
    QHash<quint16, QString>            cmdUniqueNames;
    QHash<quint16, QString>            cmdRealNames;
    QHash<quint16, QString>            cmdAppById;
    QList<quint16>                     cmdIds;
    quint32                            activeMods;
    quint32                            flags;
    quint32                            hookCmdId32;
    quint32                            tcpPayloadSize;
    quint32                            tcpFrameCmdId;
    quint8                             tcpFrameType;

    void        castPingForPeers();
    void        sendLocalInfo();
    void        loadCmds();
    void        closeByChId(const QByteArray &chId, bool peerCast);
    void        castPeerInfo(quint8 typeId);
    void        login(const QByteArray &uId);
    void        logout(const QByteArray &uId, bool reload);
    void        startCmdProc(quint32 cmdId);
    void        startModProc(const QString &modApp);
    void        addIpAction(const QString &action);
    void        castPeerStat(const QByteArray &targets, bool isDisconnecting);
    ModProcess *initModProc(const QString &modApp);
    QByteArray  genSessionId();

    // async_funcs.cpp ----

    void openSubChannel(const QByteArray &data);
    void closeSubChannel(const QByteArray &data);
    void acctDeleted(const QByteArray &data);
    void acctEdited(const QByteArray &data);
    void acctRenamed(const QByteArray &data);
    void acctDispChanged(const QByteArray &data);
    void castCatch(const QByteArray &data);
    void directDataFromPeer(const QByteArray &data);
    void p2p(const QByteArray &data);
    void closeP2P(const QByteArray &data);
    void limitedCastCatch(const QByteArray &data);
    void updateRankViaUser(const QByteArray &data);
    void addModule(const QByteArray &data);
    void rmModule(const QByteArray &data);
    void userAddedToChannel(quint16 cmdId, const QByteArray &data);
    void userRemovedFromChannel(const QByteArray &data);
    void channelDeleted(const QByteArray &data);
    void channelMemberLevelUpdated(const QByteArray &data);
    void channelRenamed(const QByteArray &data);
    void channelActiveFlagUpdated(const QByteArray &data);
    void subChannelAdded(quint16 cmdId, const QByteArray &data);
    void subChannelUpdated(quint16 cmdId, const QByteArray &data);

    //---------------------

private slots:

    void dataFromClient();
    void payloadDeleted();
    void modProcFinished();
    void cmdProcFinished(quint32 cmdId);
    void cmdProcStarted(quint32 cmdId);
    void asyncToClient(quint16 cmdId, const QByteArray &data, quint8 typeId);
    void dataToClient(quint32 cmdId, const QByteArray &data, quint8 typeId);
    void dataToCmd(quint32 cmdId, const QByteArray &data, quint8 typeId);

public:

    explicit Session(const QString &hostKey, QSslSocket *tcp, QObject *parent = nullptr);

public slots:

    void pubAsyncDataIn(quint16 cmdId, const QByteArray &data);
    void privAsyncDataIn(quint16 cmdId, const QByteArray &data);
    void connectToPeer(const QSharedPointer<SessionCarrier> &peer);
    void endSession();
    void sesRdy();
    void init();

signals:

    void killCmd16(quint16 cmdId);
    void killCmd32(quint32 cmdId);
    void asyncToPeers(quint16 cmdId, const QByteArray data);
    void connectPeers(QSharedPointer<SessionCarrier> peer);
    void setMaxSessions(quint32 value);
    void ended();
    void closeServer();
    void resServer();
    void killMods();
};

#endif // SOCKET_H
