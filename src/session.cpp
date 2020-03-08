#include "session.h"

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

QByteArray wrFrame(quint32 cmdId, const QByteArray &data, uchar dType)
{
    auto typeBa  = wrInt(dType, 8);
    auto cmdBa   = wrInt(cmdId, 32);
    auto sizeBa  = wrInt(data.size(), MAX_FRAME_BITS);

    return typeBa + cmdBa + sizeBa + data;
}

Session::Session(const QString &hostKey, QSslSocket *tcp, QObject *parent) : MemShare(parent)
{
    currentDir     = QDir::currentPath();
    hostMemKey     = hostKey;
    tcpSocket      = tcp;
    hookCmdId32    = 0;
    tcpFrameCmdId  = 0;
    tcpPayloadSize = 0;
    tcpFrameType   = 0;
    flags          = 0;
}

void Session::init()
{
    if (createSharedMem(genSessionId(), hostMemKey))
    {
        setupDataBlocks();
        wrStringToBlock(tcpSocket->peerAddress().toString(), clientIp, BLKSIZE_CLIENT_IP);

        connect(tcpSocket, &QSslSocket::disconnected, this, &Session::endSession);
        connect(tcpSocket, &QSslSocket::readyRead, this, &Session::dataFromClient);
        connect(tcpSocket, &QSslSocket::encrypted, this, &Session::sesRdy);
    }
    else
    {
        endSession();
    }
}

QByteArray Session::genSessionId()
{
    auto serial = genSerialNumber().toUtf8();
    auto sysId  = QSysInfo::machineUniqueId();

    QCryptographicHash hasher(QCryptographicHash::Sha3_224);

    hasher.addData(serial + sysId);

    return hasher.result();
}

void Session::payloadDeleted()
{
    flags &= ~ACTIVE_PAYLOAD;

    if (flags & END_SESSION_ON_PAYLOAD_DEL)
    {
        endSession();
    }
}

void Session::connectToPeer(const QSharedPointer<SessionCarrier> &peer)
{
    if (peer->sessionObj == nullptr)
    {
        qDebug() << "Session::connectToPeer() the peer session object is null.";
    }
    else if ((peer->sessionObj != this) && (flags & SESSION_RDY))
    {
        connect(peer->sessionObj, &Session::asyncToPeers, this, &Session::pubAsyncDataIn);
        connect(this, &Session::asyncToPeers, peer->sessionObj, &Session::pubAsyncDataIn);
    }
}

void Session::sesRdy()
{
    flags |= SESSION_RDY;

    auto *payload = new SessionCarrier(this);

    connect(payload, &SessionCarrier::destroyed, this, &Session::payloadDeleted);

    emit connectPeers(QSharedPointer<SessionCarrier>(payload));

    loadCmds();
    asyncToClient(ASYNC_RDY, toTEXT("\nReady!\n\n"), TEXT);
}

void Session::addIpAction(const QString &action)
{
    Query db(this);

    auto ip  = rdStringFromBlock(clientIp, BLKSIZE_CLIENT_IP);
    auto app = rdStringFromBlock(appName, BLKSIZE_APP_NAME);
    auto id  = rdFromBlock(sessionId, BLKSIZE_SESSION_ID);

    db.setType(Query::PUSH, TABLE_IPHIST);
    db.addColumn(COLUMN_IPADDR, ip);
    db.addColumn(COLUMN_LOGENTRY, action);
    db.addColumn(COLUMN_SESSION_ID, id);
    db.addColumn(COLUMN_APP_NAME, app);
    db.exec();
}

void Session::cmdProcFinished(quint32 cmdId)
{
    cmdProcesses.remove(cmdId);
    frameQueue.remove(cmdId);

    if (hookCmdId32 == cmdId)
    {
        hookCmdId32 = 0;
    }

    if (flags & END_SESSION_EMPTY_PROC)
    {
        endSession();
    }
}

void Session::cmdProcStarted(quint32 cmdId, CmdProcess *obj)
{
    cmdProcesses.insert(cmdId, obj);

    if (frameQueue.contains(cmdId))
    {
        for (auto&& frame : frameQueue[cmdId])
        {
            dataToCmd(cmdId, frame.mid(1), static_cast<quint8>(frame[0]));
        }

        frameQueue.remove(cmdId);
    }
}

void Session::endSession()
{
    emit killMods();

    logout("", false);

    if (flags & ACTIVE_PAYLOAD)
    {
        flags |= END_SESSION_ON_PAYLOAD_DEL;
    }
    else
    {   
        if (cmdProcesses.isEmpty())
        {
            addIpAction("Session Ended");
            cleanupDbConnection();

            emit ended();
        }
        else
        {
            flags |= END_SESSION_EMPTY_PROC;

            for (auto id : cmdProcesses.keys())
            {
                emit killCmd32(id);
            }
        }
    }
}

void Session::startCmdProc(quint32 cmdId)
{
    quint16 cmdId16 = toCmdId16(cmdId);
    QString modApp  = cmdAppById[cmdId16];
    QString pipe    = rdFromBlock(sessionId, BLKSIZE_USER_ID).toHex() + "-cmd-" + QString::number(cmdId);

    auto *proc = new CmdProcess(cmdId, cmdRealNames[cmdId16], modApp, sesMemKey, hostMemKey, pipe, this);

    proc->setWorkingDirectory(currentDir);
    proc->setSessionParams(sharedMem, sessionId, openWritableSubChs, &hookCmdId32);

    connect(proc, &CmdProcess::cmdProcFinished, this, &Session::cmdProcFinished);
    connect(proc, &CmdProcess::cmdProcReady, this, &Session::cmdProcStarted);
    connect(proc, &CmdProcess::pubIPC, this, &Session::asyncToPeers);
    connect(proc, &CmdProcess::privIPC, this, &Session::privAsyncDataIn);
    connect(proc, &CmdProcess::pubIPCWithFeedBack, this, &Session::asyncToPeers);
    connect(proc, &CmdProcess::pubIPCWithFeedBack, this, &Session::pubAsyncDataIn);
    connect(proc, &CmdProcess::dataToClient, this, &Session::dataToClient);

    connect(this, &Session::killCmd16, proc, &CmdProcess::killCmd16);
    connect(this, &Session::killCmd32, proc, &CmdProcess::killCmd32);

    proc->startCmdProc();
}

ModProcess *Session::initModProc(const QString &modApp)
{
    QString pipe = rdFromBlock(sessionId, BLKSIZE_USER_ID).toHex() + "-mod-" + genSerialNumber();
    quint32 rnk  = rd32BitFromBlock(hostRank);

    auto *proc = new ModProcess(modApp, sesMemKey, hostMemKey, pipe, this);

    proc->setWorkingDirectory(currentDir);
    proc->setSessionParams(&cmdUniqueNames, &cmdRealNames, &cmdAppById, &modCmdNames, &cmdIds, rnk);

    connect(proc, &ModProcess::dataToClient, this, &Session::dataToClient);
    connect(proc, &ModProcess::cmdUnloaded, this, &Session::killCmd16);

    connect(this, &Session::killMods, proc, &ModProcess::killProc);

    return proc;
}

void Session::startModProc(const QString &modApp)
{
    if (flags & LOGGED_IN)
    {
        initModProc(modApp)->loadExemptCmds();
        initModProc(modApp)->loadUserCmds();
    }
    else
    {
        initModProc(modApp)->loadPublicCmds();
    }
}

void Session::loadCmds()
{
    startModProc(QCoreApplication::applicationFilePath());

    Query db(this);

    db.setType(Query::PULL, TABLE_MODULES);
    db.addColumn(COLUMN_MOD_MAIN);
    db.exec();

    for (int i = 0; i < db.rows(); ++i)
    {
        startModProc(db.getData(COLUMN_MOD_MAIN, i).toString());
    }
}

void Session::dataToCmd(quint32 cmdId, const QByteArray &data, quint8 typeId)
{
    quint16 cmdId16 = toCmdId16(cmdId);

    if (cmdIds.contains(cmdId16))
    {
        if (cmdProcesses.contains(cmdId))
        {
            cmdProcesses[cmdId]->dataFromSession(cmdId, data, typeId);
        }
        else
        {
            if (frameQueue.contains(cmdId))
            {
                frameQueue[cmdId].append(wrInt(typeId, 8) + data);
            }
            else
            {
                QList<QByteArray> frames;

                frames.append(wrInt(typeId, 8) + data);
                frameQueue.insert(cmdId, frames);
            }

            startCmdProc(cmdId);
        }
    }
    else
    {
        dataToClient(cmdId, toTEXT("err: No such command id: " + QString::number(cmdId16) + "."), ERR);
    }
}

void Session::dataFromClient()
{
    if (flags & SESSION_RDY)
    {   
        if (flags & FRAME_RDY)
        {
            if (tcpSocket->bytesAvailable() >= tcpPayloadSize)
            {
                if (hookCmdId32 != 0)
                {
                    dataToCmd(hookCmdId32, tcpSocket->read(tcpPayloadSize), tcpFrameType);
                }
                else
                {
                    dataToCmd(tcpFrameCmdId, tcpSocket->read(tcpPayloadSize), tcpFrameType);
                }

                flags ^= FRAME_RDY;

                dataFromClient();
            }
        }
        else if (tcpSocket->bytesAvailable() >= FRAME_HEADER_SIZE)
        {
            QByteArray header = tcpSocket->read(FRAME_HEADER_SIZE);

            tcpFrameType   = static_cast<quint8>(header[0]);
            tcpFrameCmdId  = static_cast<quint32>(rdInt(header.mid(1, 4)));
            tcpPayloadSize = static_cast<quint32>(rdInt(header.mid(5)));
            flags         |= FRAME_RDY;

            dataFromClient();
        }
    }
    else
    {
        if (tcpSocket->bytesAvailable() >= CLIENT_HEADER_LEN)
        {
            if (tcpSocket->read(4) == QByteArray(SERVER_HEADER_TAG))
            {
                wrStringToBlock(fromTEXT(tcpSocket->read(BLKSIZE_APP_NAME)).trimmed(), appName, BLKSIZE_APP_NAME);

                QString     coName = fromTEXT(tcpSocket->read(272)).trimmed();
                QStringList ver    = QCoreApplication::applicationVersion().split('.');
                QByteArray  servHeader;

                servHeader.append(wrInt(0, 8));
                servHeader.append(wrInt(ver[0].toULongLong(), 16));
                servHeader.append(wrInt(ver[1].toULongLong(), 16));
                servHeader.append(wrInt(ver[2].toULongLong(), 16));
                servHeader.append(wrInt(ver[3].toULongLong(), 16));
                servHeader.append(sessionId, BLKSIZE_SESSION_ID);

                addIpAction("Session Active");

                if (tcpSocket->peerAddress().isLoopback())
                {
                    // SSL encryption is optional for locally connected clients
                    // so sesOk() can be called right away instead of starting
                    // an SSL handshake.

                    // reply value 1 means the client needs to take no further
                    // action, just await a message from the ASYNC_RDY async
                    // command id.

                    servHeader[0] = 1;

                    tcpSocket->write(servHeader);

                    sesRdy();
                }
                else
                {
                    QByteArray certBa;
                    QByteArray privBa;

                    if (getCertAndKey(coName, certBa, privBa))
                    {
                        servHeader[0] = 2;

                        // reply value 2 means the client version is acceptable
                        // but the host will now send it's Pem formatted SSL cert
                        // data in a HOST_CERT mrci frame just after sending it's
                        // header.

                        // the client must use this cert and send a STARTTLS
                        // signal when ready.

                        tcpSocket->setLocalCertificate(toSSLCert(certBa));
                        tcpSocket->setPrivateKey(toSSLKey(privBa));
                        tcpSocket->write(servHeader);

                        dataToClient(ASYNC_SYS_MSG, certBa, HOST_CERT);

                        tcpSocket->startServerEncryption();
                    }
                    else
                    {
                        servHeader[0] = 4;

                        // reply value 4 means the host was unable to load the
                        // SSL cert associated with the common name sent by the
                        // client. the session will lock out and auto close at
                        // this point.

                        tcpSocket->write(servHeader);

                        endSession();
                    }
                }
            }
            else
            {
                endSession();
            }
        }
    }
}

void Session::dataToClient(quint32 cmdId, const QByteArray &data, quint8 typeId)
{
    tcpSocket->write(wrFrame(cmdId, data, typeId));
}

void Session::asyncToClient(quint16 cmdId, const QByteArray &data, quint8 typeId)
{
    dataToClient(toCmdId32(cmdId, 0), data, typeId);
}

void Session::logout(const QByteArray &uId, bool reload)
{
    if (rd8BitFromBlock(activeUpdate))
    {
        castPeerStat(rdFromBlock(openSubChs, MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL), true);
    }

    emit asyncToPeers(ASYNC_CLOSE_P2P, rdFromBlock(sessionId, BLKSIZE_SESSION_ID));

    memset(userId, 0, BLKSIZE_USER_ID);
    memset(userName, 0, BLKSIZE_USER_NAME);
    memset(displayName, 0, BLKSIZE_DISP_NAME);
    memset(openSubChs, 0, MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL);
    memset(openWritableSubChs, 0, MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL);
    memset(chList, 0, MAX_CHANNELS_PER_USER * BLKSIZE_CHANNEL_ID);

    wr32BitToBlock(0, hostRank);
    wr8BitToBlock(0, activeUpdate);
    wr8BitToBlock(0, chOwnerOverride);

    flags &= ~LOGGED_IN;

    if (uId.isEmpty())
    {
        if (reload)
        {
            loadCmds();
        }
    }
    else
    {
        login(uId);
    }
}

void Session::login(const QByteArray &uId)
{
    if (flags & LOGGED_IN)
    {
        logout(uId, true);
    }
    else
    {
        Query db(this);

        db.setType(Query::PULL, TABLE_USERS);
        db.addColumn(COLUMN_USERNAME);
        db.addColumn(COLUMN_HOST_RANK);
        db.addColumn(COLUMN_DISPLAY_NAME);
        db.addCondition(COLUMN_USER_ID, uId);
        db.exec();

        wrToBlock(uId, userId, BLKSIZE_USER_ID);
        wrStringToBlock(db.getData(COLUMN_USERNAME).toString(), userName, BLKSIZE_USER_NAME);
        wrStringToBlock(db.getData(COLUMN_DISPLAY_NAME).toString(), displayName, BLKSIZE_DISP_NAME);
        wr32BitToBlock(db.getData(COLUMN_HOST_RANK).toUInt(), hostRank);

        db.setType(Query::PULL, TABLE_CH_MEMBERS);
        db.addColumn(COLUMN_CHANNEL_ID);
        db.addCondition(COLUMN_USER_ID, uId);
        db.exec();

        memset(chList, 0, MAX_CHANNELS_PER_USER * BLKSIZE_CHANNEL_ID);

        for (int i = 0; i < db.rows(); ++i)
        {
            QByteArray chId = wrInt(db.getData(COLUMN_CHANNEL_ID, i).toULongLong(), 64);

            addBlockToBlockset(chId.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID);
        }

        flags |= LOGGED_IN;

        sendLocalInfo();
        loadCmds();
    }
}

void Session::sendLocalInfo()
{
    QByteArray frame = createPeerInfoFrame();

    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_EMAIL);
    db.addColumn(COLUMN_EMAIL_VERIFIED);
    db.addCondition(COLUMN_USER_ID, rdFromBlock(userId, BLKSIZE_USER_ID));
    db.exec();

    frame.append(fixedToTEXT(db.getData(COLUMN_EMAIL).toString(), BLKSIZE_EMAIL_ADDR));
    frame.append(rdFromBlock(hostRank, BLKSIZE_HOST_RANK));

    if (db.getData(COLUMN_EMAIL_VERIFIED).toBool())
    {
        frame.append(static_cast<char>(0x01));
    }
    else
    {
        frame.append(static_cast<char>(0x00));
    }

    dataToClient(ASYNC_SYS_MSG, frame, MY_INFO);
}

void Session::castPeerStat(const QByteArray &oldSubIds, bool isDisconnecting)
{
    if (rd8BitFromBlock(activeUpdate))
    {
        // format: [54bytes(chIds)][1byte(typeId)][rest-of-bytes(PEER_STAT)]

        QByteArray typeId   = wrInt(PEER_STAT, 8);
        QByteArray sesId    = rdFromBlock(sessionId, BLKSIZE_SESSION_ID);
        QByteArray openSubs = rdFromBlock(openSubChs, MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL);
        QByteArray dc;

        if (isDisconnecting)
        {
            dc = QByteArray(1, 0x01);
        }
        else
        {
            dc = QByteArray(1, 0x00);
        }

        emit asyncToPeers(ASYNC_LIMITED_CAST, oldSubIds + typeId + sesId + openSubs + dc);
    }
}

void Session::castPeerInfo(quint8 typeId)
{
    if (rd8BitFromBlock(activeUpdate))
    {
        // format: [54bytes(chIds)][1byte(typeId)][rest-of-bytes(PEER_INFO)]

        QByteArray openSubs = rdFromBlock(openWritableSubChs, MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL);
        QByteArray typeIdBa = wrInt(typeId, 8);
        QByteArray frame    = createPeerInfoFrame();

        emit asyncToPeers(ASYNC_LIMITED_CAST, openSubs + typeIdBa + frame);
    }
}

void Session::castPingForPeers()
{
    castPeerInfo(PING_PEERS);
}

void Session::closeByChId(const QByteArray &chId, bool peerCast)
{
    QByteArray oldSubChs;

    if (peerCast)
    {
        oldSubChs = QByteArray(openSubChs, MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL);
    }

    rmLikeBlkFromBlkset(chId, openWritableSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL);

    if (rmLikeBlkFromBlkset(chId, openSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL) && peerCast)
    {
        castPeerStat(oldSubChs, false);
    }
}

void Session::privAsyncDataIn(quint16 cmdId, const QByteArray &data)
{
    sharedMem->lock();

    if (cmdId == ASYNC_END_SESSION)
    {
        endSession();
    }
    else if (cmdId == ASYNC_USER_LOGIN)
    {
        login(data);
    }
    else if (cmdId == ASYNC_LOGOUT)
    {
        logout("", true);
    }
    else if (cmdId == ASYNC_EXIT)
    {
        emit closeServer();
    }
    else if (cmdId == ASYNC_RESTART)
    {
        emit resServer();
    }
    else if (cmdId == ASYNC_MAXSES)
    {
        emit setMaxSessions(static_cast<quint32>(rdInt(data)));
    }
    else if (cmdId == ASYNC_PING_PEERS)
    {
        castPingForPeers();
    }
    else if (cmdId == ASYNC_OPEN_SUBCH)
    {
        openSubChannel(data);
    }
    else if (cmdId == ASYNC_CLOSE_SUBCH)
    {
        closeSubChannel(data);
    }
    else if (cmdId == ASYNC_SET_DIR)
    {
        currentDir = fromTEXT(data);
    }
    else if (cmdId == ASYNC_DEBUG_TEXT)
    {
        qDebug() << fromTEXT(data);
    }

    sharedMem->unlock();
}

void Session::pubAsyncDataIn(quint16 cmdId, const QByteArray &data)
{
    sharedMem->lock();

    if (cmdId == ASYNC_USER_DELETED)
    {
        acctDeleted(data);
    }
    else if (cmdId == ASYNC_CAST)
    {
        castCatch(data);
    }
    else if (cmdId == ASYNC_TO_PEER)
    {
        directDataFromPeer(data);
    }
    else if (cmdId == ASYNC_P2P)
    {
        p2p(data);
    }
    else if (cmdId == ASYNC_CLOSE_P2P)
    {
        closeP2P(data);
    }
    else if (cmdId == ASYNC_LIMITED_CAST)
    {
        limitedCastCatch(data);
    }
    else if (cmdId == ASYNC_RW_MY_INFO)
    {
        acctEdited(data);
    }
    else if (cmdId == ASYNC_USER_RENAMED)
    {
        acctRenamed(data);
    }
    else if (cmdId == ASYNC_DISP_RENAMED)
    {
        acctDispChanged(data);
    }
    else if (cmdId == ASYNC_USER_RANK_CHANGED)
    {
        updateRankViaUser(data);
    }
    else if (cmdId == ASYNC_CMD_RANKS_CHANGED)
    {
        loadCmds();
    }
    else if (cmdId == ASYNC_ENABLE_MOD)
    {
        addModule(data);
    }
    else if (cmdId == ASYNC_DISABLE_MOD)
    {
        rmModule(data);
    }
    else if ((cmdId == ASYNC_NEW_CH_MEMBER) || (cmdId == ASYNC_INVITED_TO_CH) ||
             (cmdId == ASYNC_INVITE_ACCEPTED))
    {
        userAddedToChannel(cmdId, data);
    }
    else if (cmdId == ASYNC_RM_CH_MEMBER)
    {
        userRemovedFromChannel(data);
    }
    else if (cmdId == ASYNC_DEL_CH)
    {
        channelDeleted(data);
    }
    else if (cmdId == ASYNC_MEM_LEVEL_CHANGED)
    {
        channelMemberLevelUpdated(data);
    }
    else if (cmdId == ASYNC_RENAME_CH)
    {
        channelRenamed(data);
    }
    else if (cmdId == ASYNC_CH_ACT_FLAG)
    {
        channelActiveFlagUpdated(data);
    }
    else if ((cmdId == ASYNC_NEW_SUB_CH) || (cmdId == ASYNC_RENAME_SUB_CH))
    {
        subChannelAdded(cmdId, data);
    }
    else if ((cmdId == ASYNC_RM_SUB_CH) || (cmdId == ASYNC_SUB_CH_LEVEL_CHG) ||
             (cmdId == ASYNC_RM_RDONLY) || (cmdId == ASYNC_ADD_RDONLY))
    {
        subChannelUpdated(cmdId, data);
    }

    sharedMem->unlock();
}
