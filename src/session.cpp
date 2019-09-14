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

Session::Session(QObject *parent) : QObject(parent)
{
    chIds           = QByteArray(54, static_cast<char>(0));
    rwShared        = new RWSharedObjs(this);
    shared          = new SharedObjs(this);
    exeDebugInfo    = new QSharedMemory(this);
    ipcServ         = nullptr;
    slaveProc       = nullptr;
    ipcLink         = nullptr;
    tcpSocket       = nullptr;
    activeUpdate    = false;
    chOwnerOverride = false;
    exeCrashCount   = 0;
    clientMajor     = 0;
    clientMinor     = 0;
    clientPatch     = 0;
    ipcFrameCmdId   = 0;
    ipcFrameSize    = 0;
    ipcFrameType    = 0;
    tcpFrameCmdId   = 0;
    tcpFrameSize    = 0;
    tcpFrameType    = 0;
    hostRank        = 0;
    flags           = 0;

    shared->clientMajor     = &clientMajor;
    shared->clientMinor     = &clientMinor;
    shared->clientPatch     = &clientPatch;
    shared->groupName       = &groupName;
    shared->userName        = &userName;
    shared->displayName     = &displayName;
    shared->appName         = &appName;
    shared->sessionAddr     = &peerIp;
    shared->sessionId       = &sessionId;
    shared->userId          = &userId;
    shared->chIds           = &chIds;
    shared->wrAbleChIds     = &wrAbleChIds;
    shared->p2pAccepted     = &p2pAccepted;
    shared->p2pPending      = &p2pPending;
    shared->activeUpdate    = &activeUpdate;
    shared->chList          = &chList;
    shared->chOwnerOverride = &chOwnerOverride;
    shared->hostRank        = &hostRank;

    rwShared->clientMajor     = &clientMajor;
    rwShared->clientMinor     = &clientMinor;
    rwShared->clientPatch     = &clientPatch;
    rwShared->groupName       = &groupName;
    rwShared->userName        = &userName;
    rwShared->displayName     = &displayName;
    rwShared->appName         = &appName;
    rwShared->sessionAddr     = &peerIp;
    rwShared->sessionId       = &sessionId;
    rwShared->userId          = &userId;
    rwShared->chIds           = &chIds;
    rwShared->wrAbleChIds     = &wrAbleChIds;
    rwShared->p2pAccepted     = &p2pAccepted;
    rwShared->p2pPending      = &p2pPending;
    rwShared->activeUpdate    = &activeUpdate;
    rwShared->chList          = &chList;
    rwShared->chOwnerOverride = &chOwnerOverride;
    rwShared->hostRank        = &hostRank;
}

void Session::genSessionId()
{
    QByteArray serial = genSerialNumber().toUtf8();
    QByteArray sysId  = QSysInfo::machineUniqueId();

    QCryptographicHash hasher(QCryptographicHash::Sha3_224);

    hasher.addData(serial + sysId);

    sessionId = hasher.result();
}

void Session::initAsMain(QSslSocket *tcp)
{
    genSessionId();

    tcpSocket = tcp;
    slaveProc = new QProcess(this);
    ipcServ   = new QLocalServer(this);
    peerIp    = tcp->peerAddress().toString();
    pipeName  = pipesPath() + "/" + sessionId.toHex();

    if (QFile::exists(pipeName))
    {
        QFile::remove(pipeName);
    }

    exeDebugInfo->setKey(sessionId.toHex());
    exeDebugInfo->create(EXE_DEBUG_INFO_SIZE);

    auto buffSize = static_cast<uint>(qPow(2, MAX_FRAME_BITS) - 1) + (MAX_FRAME_BITS / 8) + 2;
    //                                max_data_size_per_frame + size_of_size_bytes + size_of_cmd_id

    tcp->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, buffSize);
    tcp->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, buffSize);

    connect(slaveProc, &QProcess::readyReadStandardError, this, &Session::sendStderr);
    connect(slaveProc, &QProcess::readyReadStandardOutput, this, &Session::sendStdout);
    connect(slaveProc, &QProcess::errorOccurred, this, &Session::exeError);
    connect(slaveProc, &QProcess::started, this, &Session::exeStarted);
    connect(slaveProc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(exeFinished(int,QProcess::ExitStatus)));

    connect(tcpSocket, &QSslSocket::disconnected, this, &Session::endSession);
    connect(tcpSocket, &QSslSocket::readyRead, this, &Session::dataFromClient);
    connect(tcpSocket, &QSslSocket::encrypted, this, &Session::run);

    connect(ipcServ, &QLocalServer::newConnection, this, &Session::newIPCLink);
}

void Session::startAsSlave(const QStringList &args)
{
    sessionId   = QByteArray::fromHex(getParam("-session_id", args).toUtf8());
    clientMajor = getParam("-client_major", args).toUShort();
    clientMinor = getParam("-client_minor", args).toUShort();
    clientPatch = getParam("-client_patch", args).toUShort();
    peerIp      = getParam("-session_ipaddr", args);
    appName     = getParam("-app_name", args);
    pipeName    = pipesPath() + "/" + sessionId.toHex();
    ipcLink     = new QLocalSocket(this);
    executor    = new CmdExecutor(rwShared, shared, exeDebugInfo);

    exeDebugInfo->setKey(sessionId.toHex());
    exeDebugInfo->attach();

    auto *exeThr = new QThread();

    connect(exeThr, &QThread::finished, executor, &CmdExecutor::deleteLater);
    connect(exeThr, &QThread::finished, exeThr, &QThread::deleteLater);
    connect(exeThr, &QThread::started, executor, &CmdExecutor::buildCmdLoaders);

    connect(executor, &CmdExecutor::okToDelete, this, &Session::closeInstance);
    connect(executor, &CmdExecutor::okToDelete, exeThr, &QThread::quit);
    connect(executor, &CmdExecutor::logout, this, &Session::logout);
    connect(executor, &CmdExecutor::authOk, this, &Session::authOk);
    connect(executor, &CmdExecutor::endSession, this, &Session::endSession);
    connect(executor, &CmdExecutor::dataToSession, this, &Session::dataToClient);

    connect(this, &Session::closeExe, executor, &CmdExecutor::close);
    connect(this, &Session::loadCommands, executor, &CmdExecutor::buildCommands);
    connect(this, &Session::dataToCommand, executor, &CmdExecutor::exeCmd);
    connect(this, &Session::unloadModFile, executor, &CmdExecutor::unloadModFile);
    connect(this, &Session::loadModFile, executor, &CmdExecutor::loadModFile);

    connect(ipcLink, &QLocalSocket::connected, this, &Session::ipcConnected);
    connect(ipcLink, &QLocalSocket::disconnected, this, &Session::ipcDisconnected);
    connect(ipcLink, &QLocalSocket::readyRead, this, &Session::dataFromIPC);
    connect(ipcLink, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(ipcError(QLocalSocket::LocalSocketError)));

    serializeThread(exeThr);

    executor->moveToThread(exeThr);
    exeThr->start();

    QTimer::singleShot(100, this, SLOT(run()));
}

void Session::modLoadCrashCheck(const QString &crashInfo)
{
    if (crashInfo.contains("loadModLib()"))
    {
        int pos = crashInfo.indexOf("path: ");

        if (pos != -1)
        {
            QString path = crashInfo.mid(pos + 7).trimmed();

            Query db(this);

            db.setType(Query::UPDATE, TABLE_MODULES);
            db.addColumn(COLUMN_LOCKED, true);
            db.addCondition(COLUMN_MOD_MAIN, path);
            db.exec();

            qDebug() << "Module: '" << path << "' auto locked due to a crash while loading.";
        }
    }
}

void Session::rdExeDebug()
{
    if (exeDebugInfo->isAttached())
    {
        exeDebugInfo->lock();

        QByteArray data = QByteArray(static_cast<char*>(exeDebugInfo->data()), EXE_DEBUG_INFO_SIZE);

        data.replace(QByteArray(2, static_cast<char>(0)), QByteArray());

        emit dataToClient(ASYNC_SYS_MSG, toTEXT("\ndebug info:\n"), TEXT);
        emit dataToClient(ASYNC_SYS_MSG, data + toTEXT("\n"), TEXT);

        qDebug() << "Debug info generated: " + fromTEXT(data);

        modLoadCrashCheck(fromTEXT(data));

        exeDebugInfo->unlock();
    }
}

bool Session::isSlave()
{
    return (tcpSocket == nullptr);
}

bool Session::isMain()
{
    return (tcpSocket != nullptr);
}

void Session::payloadDeleted()
{
    if (isSlave())
    {
        qDebug() << "Session::payloadDeleted() called while running in slave mode.";
    }
    else
    {
        flags &= ~ACTIVE_PAYLOAD;

        if (flags & END_SESSION_ON_PAYLOAD_DEL)
        {
            endSession();
        }
    }
}

void Session::connectToPeer(const QSharedPointer<SessionCarrier> &peer)
{
    if (isSlave())
    {
        qDebug() << "Session::connectToPeer() called while running in slave mode.";
    }
    else if (peer->sessionObj == nullptr)
    {
        qDebug() << "Session::connectToPeer() the peer session object is null.";
    }
    else if ((peer->sessionObj != this) && (flags & IPC_LINK_OK))
    {
        connect(peer->sessionObj, &Session::backendToPeers, this, &Session::peersDataIn);
        connect(this, &Session::backendToPeers, peer->sessionObj, &Session::peersDataIn);
    }
}

void Session::run()
{
    flags &= ~SSL_HOLD;

    if (isSlave())
    {
        ipcLink->connectToServer(pipeName);

        QTimer::singleShot(IPC_PREP_TIME, this, SLOT(newIPCTimeout()));
    }
    else if (!ipcServ->listen(pipeName))
    {
        qDebug() << "Session::run() unable to listen on pipe name: " + pipeName + " reason: " + ipcServ->errorString();

        dataToClient(ASYNC_SYS_MSG, toTEXT("\nsystem err: Unable to create an IPC pipe for the command executor, ending the session.\n"), ERR);
        endSession();
    }
    else
    {
        QStringList args;

        args.append("-executor");
        args.append("-session_id");
        args.append(sessionId.toHex());
        args.append("-client_major");
        args.append(QString::number(clientMajor));
        args.append("-client_minor");
        args.append(QString::number(clientMinor));
        args.append("-client_patch");
        args.append(QString::number(clientPatch));
        args.append("-session_ipaddr");
        args.append(peerIp);
        args.append("-app_name");
        args.append(appName);

        slaveProc->start(QCoreApplication::applicationFilePath(), args);

        dataToClient(ASYNC_SYS_MSG, toTEXT("Attempting to start a new command executor.\n"), TEXT);

        QTimer::singleShot(IPC_PREP_TIME, this, SLOT(newIPCTimeout()));
    }
}

void Session::ipcOk()
{
    if (isSlave())
    {
        qDebug() << "Session::ipcOk() called while running in slave mode.";
    }
    else
    {
        flags |= IPC_LINK_OK;
        flags |= ACTIVE_PAYLOAD;

        auto *payload = new SessionCarrier(this);

        connect(ipcLink, &QLocalSocket::readyRead, this, &Session::dataFromIPC);
        connect(ipcLink, &QLocalSocket::disconnected, this, &Session::ipcDisconnected);
        connect(ipcLink, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(ipcError(QLocalSocket::LocalSocketError)));

        connect(payload, &SessionCarrier::destroyed, this, &Session::payloadDeleted);

        emit connectPeers(QSharedPointer<SessionCarrier>(payload));

        dataToClient(ASYNC_SYS_MSG, toTEXT("IPC connection successfully established.\n"), TEXT);

        if (!userId.isEmpty())
        {
            ipcLink->write(wrFrame(ASYNC_RESTORE_AUTH, userId, PRIV_IPC));
        }
        else
        {
            ipcLink->write(wrFrame(ASYNC_PUBLIC_AUTH, QByteArray(), PRIV_IPC));
        }
    }
}

void Session::newIPCTimeout()
{
    if (isSlave())
    {
        if (ipcLink->state() != QLocalSocket::ConnectedState)
        {
            QCoreApplication::exit(PIPE_CONNECT_TIMEOUT);
        }
    }
    else if (ipcLink == nullptr)
    {
        dataToClient(ASYNC_SYS_MSG, toTEXT("\nsystem err: Timed out waiting for the command executor to request an IPC link with the session.\n"), ERR);
        endSession();
    }
}

void Session::newIPCLink()
{
    if (isSlave())
    {
        qDebug() << "Session::newIPCLink() called while running in slave mode.";
    }
    else
    {
        if (ipcLink != nullptr)
        {
            qDebug() << "Session::newIPCLink() another local socket tried to connect to the session while there is already an active connection. session id: " + sessionId.toHex();

            ipcServ->nextPendingConnection()->deleteLater();
        }
        else
        {
            ipcLink = ipcServ->nextPendingConnection();

            QTimer::singleShot(IPC_PREP_TIME, this, SLOT(ipcOk()));
        }
    }
}

void Session::exeStarted()
{
    if (isSlave())
    {
        qDebug() << "Session::exeStarted() called while running in slave mode.";
    }
    else
    {
        dataToClient(ASYNC_SYS_MSG, toTEXT("Command executor successfully started, awaiting an IPC request.\n"), TEXT);
    }
}

void Session::ipcConnected()
{
    if (isSlave())
    {
        flags |= IPC_LINK_OK;
    }
}

void Session::ipcDisconnected()
{
    flags &= ~IPC_LINK_OK;

    if (isMain())
    {
        ipcLink = nullptr;
    }
}

void Session::exeFinished(int ret, QProcess::ExitStatus status)
{
    if (isSlave())
    {
        qDebug() << "Session::exeFinished() called while running in slave mode.";
    }
    else if (flags & EXPECTED_TERM)
    {
        endSession();
    }
    else if ((status == QProcess::NormalExit) && (ret == FAILED_TO_OPEN_PIPE))
    {
        dataToClient(ASYNC_EXE_CRASH, toTEXT("\nsystem err: The command executor could not open a new pipe for listening.\n"), ERR);
        endSession();
    }
    else if ((status == QProcess::NormalExit) && (ret == PIPE_CONNECT_TIMEOUT))
    {
        dataToClient(ASYNC_EXE_CRASH, toTEXT("\nsystem err: The command executor timed out waiting for the session to acknowledge the IPC request.\n"), ERR);
        endSession();
    }
    else
    {
        int msecSinceLastCrash = lastExeCrash.msecsTo(QTime::currentTime());

        if ((msecSinceLastCrash <= 5000) || lastExeCrash.isNull())
        {
            // only count executor crashes within 5sec of each other. any longer than that
            // can be considered a recoverable session.

            exeCrashCount++;
        }
        else
        {
            lastExeCrash  = QTime();
            exeCrashCount = 0;
        }

        if (exeCrashCount <= EXE_CRASH_LIMIT)
        {
            ipcServ->close();

            dataToClient(ASYNC_EXE_CRASH, toTEXT("\nsystem err: The command executor has stopped unexpectedly.\n"), ERR);
            rdExeDebug();
            dataToClient(ASYNC_SYS_MSG, toTEXT("\nAttempting to restart the executor...\n\n"), TEXT);
            run();
        }
        else
        {
            // if there is the amount of executor crashes defined in EXE_CRASH_LIMIT
            // within 5sec of each other then the session can no longer be considered
            // recoverable so it will be killed to prevent an infinite loop.

            dataToClient(ASYNC_EXE_CRASH, toTEXT("\nsystem err: The command executor has crashed too many times, ending the session.\n"), ERR);
            endSession();
        }
    }
}

void Session::exeError(QProcess::ProcessError err)
{
    if (isSlave())
    {
        qDebug() << "Session::exeError() called while running in slave mode.";
    }
    else if (err == QProcess::FailedToStart)
    {
        qDebug() << "\nsystem err: Could not start the command executor. reason: " + slaveProc->errorString() + ".\n";

        dataToClient(ASYNC_SYS_MSG, toTEXT("\nsystem err: Could not start the command executor. details are logged for the host admin.\n"), ERR);
        endSession();
    }
    else
    {
        qDebug() << "Session:: exeError() " << slaveProc->errorString();
    }
}

void Session::ipcError(QLocalSocket::LocalSocketError socketError)
{
    if (socketError != QLocalSocket::PeerClosedError)
    {
        if (isSlave())
        {
            qDebug() << "Session::ipcError() slave mode. socketError: " + QString::number(socketError) + " session id: " + sessionId.toHex();
        }
        else
        {
            qDebug() << "Session::ipcError() main mode. socketError: " + QString::number(socketError) + " session id: " + sessionId.toHex();
        }
    }
}

void Session::addIpAction(const QString &action)
{
    Query db(this);

    db.setType(Query::PUSH, TABLE_IPHIST);
    db.addColumn(COLUMN_IPADDR, peerIp);
    db.addColumn(COLUMN_LOGENTRY, action);
    db.addColumn(COLUMN_SESSION_ID, sessionId);
    db.addColumn(COLUMN_CLIENT_VER, QString::number(clientMajor) + "." +
                                    QString::number(clientMinor) + "." +
                                    QString::number(clientPatch));
    db.exec();
}

void Session::closeInstance()
{
    if (isMain())
    {
        qDebug() << "Session::closeInstance() called while running on the main process.";
    }
    else
    {   
        QCoreApplication::exit(0);
    }
}

void Session::endSession()
{
    if (isSlave())
    {
        ipcLink->write(wrFrame(ASYNC_END_SESSION, QByteArray(), PRIV_IPC));
    }
    else
    {
        if (activeUpdate)
        {
            QByteArray castHeader = chIds + wrInt(PEER_STAT, 8);
            QByteArray data       = toPEER_STAT(sessionId, chIds, true);

            emit backendToPeers(ASYNC_LIMITED_CAST, castHeader + data);
        }

        if (flags & ACTIVE_PAYLOAD)
        {
            flags |= END_SESSION_ON_PAYLOAD_DEL;
        }
        else
        {
            if (flags & IPC_LINK_OK)
            {
                flags |= EXPECTED_TERM;

                ipcLink->write(wrFrame(ASYNC_END_SESSION, QByteArray(), PRIV_IPC));
            }
            else
            {
                addIpAction("Session Ended");
                cleanupDbConnection();

                emit ended();
            }
        }
    }
}

void Session::dataFromIPC()
{   
    if (flags & IPC_FRAME_RDY)
    {
        if (ipcLink->bytesAvailable() >= ipcFrameSize)
        {
            QByteArray data = ipcLink->read(ipcFrameSize);

            if (isSlave())
            {
                // data from Session object in main process.

                if ((ipcFrameType == PRIV_IPC) || (ipcFrameType == PUB_IPC))
                {
                    backendDataIn(ipcFrameCmdId, data);
                }
                else
                {
                    emit dataToCommand(ipcFrameCmdId, data, ipcFrameType);
                }
            }
            else if (isMain())
            {
                // data from Session object in slave process.

                if (ipcFrameType == PRIV_IPC)
                {
                    backendDataIn(ipcFrameCmdId, data);
                }
                else if ((ipcFrameType == PUB_IPC) || (ipcFrameType == PUB_IPC_WITH_FEEDBACK))
                {
                    emit backendToPeers(ipcFrameCmdId, data);
                }
                else
                {
                    dataToClient(ipcFrameCmdId, data, ipcFrameType);
                }
            }

            flags ^= IPC_FRAME_RDY;

            dataFromIPC();
        }
    }
    else if (ipcLink->bytesAvailable() >= FRAME_HEADER_SIZE)
    {
        QByteArray header = ipcLink->read(FRAME_HEADER_SIZE);

        ipcFrameType  = static_cast<uchar>(header[0]);
        ipcFrameCmdId = static_cast<quint16>(rdInt(header.mid(1, 2)));
        ipcFrameSize  = static_cast<uint>(rdInt(header.mid(3)));
        flags        |= IPC_FRAME_RDY;

        dataFromIPC();
    }
}

void Session::dataFromClient()
{
    if ((flags & IPC_LINK_OK) && !(flags & SSL_HOLD))
    {   
        if (flags & TCP_FRAME_RDY)
        {
            if (tcpSocket->bytesAvailable() >= tcpFrameSize)
            {
                ipcLink->write(tcpSocket->read(tcpFrameSize));

                flags ^= TCP_FRAME_RDY;

                dataFromClient();
            }
        }
        else if (tcpSocket->bytesAvailable() >= FRAME_HEADER_SIZE)
        {
            QByteArray header = tcpSocket->peek(FRAME_HEADER_SIZE);

            tcpFrameType  = static_cast<uchar>(header[0]);
            tcpFrameCmdId = static_cast<quint16>(rdInt(header.mid(1, 2)));
            tcpFrameSize  = static_cast<uint>(rdInt(header.mid(3)) + FRAME_HEADER_SIZE);

            if ((tcpFrameType == PUB_IPC)    || (tcpFrameType == PRIV_IPC) ||
                (tcpFrameType == PING_PEERS) || (tcpFrameType == PUB_IPC_WITH_FEEDBACK))
            {
                // for obvious security reasons, TCP clients should not be allowed
                // to send any of the PrivateTypeID data frames. infact, it is logged
                // as suspicious behaviour that admins can monitor for.

                addIpAction("Suspicious action: client attempted to send a private TypeID: " + QString::number(tcpFrameType));

                tcpSocket->readAll(); // kill the frame.
            }
            else
            {
                flags |= TCP_FRAME_RDY;
            }

            dataFromClient();
        }
    }
    else if (!(flags & VER_OK))
    {
        if (tcpSocket->bytesAvailable() >= CLIENT_HEADER_LEN)
        {
            if (tcpSocket->read(4) == QByteArray(SERVER_HEADER_TAG))
            {
                clientMajor = static_cast<ushort>(rdInt(tcpSocket->read(2)));
                clientMinor = static_cast<ushort>(rdInt(tcpSocket->read(2)));
                clientPatch = static_cast<ushort>(rdInt(tcpSocket->read(2)));
                appName     = fromTEXT(tcpSocket->read(128)).trimmed();

                QString     coName = fromTEXT(tcpSocket->read(272)).trimmed();
                QStringList ver    = QCoreApplication::applicationVersion().split('.');
                QByteArray  servHeader;

                servHeader.append(wrInt(0, 8));
                servHeader.append(wrInt(ver[0].toULongLong(), 16));
                servHeader.append(wrInt(ver[1].toULongLong(), 16));
                servHeader.append(wrInt(ver[2].toULongLong(), 16));
                servHeader.append(sessionId);

                if (clientMajor == 1)
                {
                    flags |= VER_OK;

                    addIpAction("Session Active");

                    if (tcpSocket->peerAddress().isLoopback())
                    {
                        // SSL encryption is optional for locally connected clients
                        // so run() can be called right away instead of starting
                        // an SSL handshake.

                        // reply value 1 means the client version is acceptable
                        // and the client needs to take no further action, just
                        // await an IDLE mrci frame from the host to indicate
                        // that it is ready to take commands.

                        servHeader[0] = 1;

                        tcpSocket->write(servHeader);

                        run();
                    }
                    else
                    {
                        flags |= SSL_HOLD;

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
                    // replay value 3 means the client version is not supported
                    // by the host and the session will end after sending the
                    // header.

                    servHeader[0] = 3;

                    addIpAction("Client Rejected");

                    tcpSocket->write(servHeader);

                    endSession();
                }
            }
            else
            {
                endSession();
            }
        }
    }
}

void Session::dataToClient(quint16 cmdId, const QByteArray &data, uchar typeId)
{
    if (isSlave() && (flags & IPC_LINK_OK))
    {
        if (typeId == PUB_IPC_WITH_FEEDBACK)
        {
            backendDataIn(cmdId, data);
        }

        ipcLink->write(wrFrame(cmdId, data, typeId));
    }
    else if (isMain())
    {
        tcpSocket->write(wrFrame(cmdId, data, typeId));
    }
}

void Session::peersDataIn(quint16 cmdId, const QByteArray &data)
{
    if (isSlave())
    {
        qDebug() << "Session::peersDataIn() called while running in slave mode.";
    }
    else if (flags & IPC_LINK_OK)
    {
        ipcLink->write(wrFrame(cmdId, data, PRIV_IPC));
    }
}

void Session::logout()
{
    if (isMain())
    {
        qDebug() << "Session::logout() called while running on the main process.";
    }
    else
    {
        userName.clear();
        groupName.clear();
        displayName.clear();
        userId.clear();
        chIds.clear();
        chList.clear();

        hostRank = 0;

        dataToClient(ASYNC_LOGOUT, QByteArray(), PRIV_IPC);

        emit loadCommands();
    }
}

void Session::authOk()
{
    if (isMain())
    {
        qDebug() << "Session:authOk() called while running on the main process.";
    }
    else
    {
        if (!userName.isEmpty())
        {
            Query db(this);

            db.setType(Query::PULL, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_CHANNEL_NAME);
            db.addCondition(COLUMN_USERNAME, userName);
            db.exec();

            chList.clear();

            for (int i = 0; i < db.rows(); ++i)
            {
                chList.append(db.getData(COLUMN_CHANNEL_NAME, i).toString().toLower());
            }

            dataToClient(ASYNC_USER_LOGIN, userId, PRIV_IPC);
            sendLocalInfo();
        }

        emit loadCommands();
    }
}

void Session::castPeerInfo()
{
    if (isMain())
    {
        qDebug() << "Session::castPeerInfo() called while running on the main process.";
    }
    else
    {
        if (activeUpdate)
        {
            // format: [54bytes(chIds)][1byte(typeId)][rest-of-bytes(PEER_INFO)]

            QByteArray castHeader = chIds + wrInt(PEER_INFO, 8);
            QByteArray data       = toPEER_INFO(shared);

            dataToClient(ASYNC_LIMITED_CAST, castHeader + data, PUB_IPC);
        }
    }
}

void Session::sendLocalInfo()
{
    if (isMain())
    {
        qDebug() << "Session::sendLocalInfo() called while runnning on the main process.";
    }
    else
    {
        dataToClient(ASYNC_SYS_MSG, toMY_INFO(shared), MY_INFO);
    }
}

void Session::backendDataIn(quint16 cmdId, const QByteArray &data)
{
    if (flags & IPC_LINK_OK)
    {
        if (isMain())
        {
            if (cmdId == ASYNC_END_SESSION)
            {
                endSession();
            }
            else if (cmdId == ASYNC_USER_LOGIN)
            {
                userId = data;
            }
            else if (cmdId == ASYNC_LOGOUT)
            {
                userId.clear();
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
                emit setMaxSessions(static_cast<uint>(rdInt(data)));
            }
            else if (cmdId == ASYNC_DISABLE_MOD)
            {   
                emit delayedModDel(fromTEXT(data));
            }
        }
        else
        {
            if (cmdId == ASYNC_END_SESSION)
            {
                p2pAccepted.clear();
                p2pPending.clear();

                dataToClient(ASYNC_CLOSE_P2P, sessionId, PUB_IPC);

                emit closeExe();
            }
            else if (cmdId == ASYNC_USER_DELETED)
            {
                if (!userName.isEmpty())
                {
                    if (noCaseMatch(userName, fromTEXT(data)))
                    {
                        logout();
                        dataToClient(ASYNC_SYS_MSG, toTEXT("\nsystem: your session was forced to logout because your account was deleted.\n"), TEXT);
                        dataToClient(ASYNC_USER_DELETED, data, TEXT);
                    }
                }
            }
            else if (cmdId == ASYNC_CAST)
            {
                // format: [54bytes(wrAbleChIds)][1byte(typeId)][rest-of-bytes(payload)]

                if (matchChs(chIds, QByteArray::fromRawData(data.data(), 54)))
                {
                    dataToClient(cmdId, data.mid(55), static_cast<uchar>(data[54]));
                }
            }
            else if (cmdId == ASYNC_TO_PEER)
            {
                // format: [28bytes(sessionId)][1byte(typeId)][rest-of-bytes(payload)]

                if (QByteArray::fromRawData(data.data(), 28) == sessionId)
                {
                    dataToClient(cmdId, data.mid(29), static_cast<uchar>(data[28]));
                }
            }
            else if (cmdId == ASYNC_P2P)
            {
                // format: [28bytes(dst_sessionId)][28bytes(src_sessionId)][1byte(typeId)][rest-of-bytes(payload)]

                if (QByteArray::fromRawData(data.data(), 28) == sessionId)
                {
                    QByteArray src = data.mid(28, 28);

                    if (data[56] == P2P_REQUEST)
                    {
                        if (!p2pPending.contains(src) && !p2pAccepted.contains(src))
                        {
                            p2pPending.append(src);

                            dataToClient(cmdId, data.mid(57), P2P_REQUEST);
                        }
                    }
                    else if (data[56] == P2P_OPEN)
                    {
                        if (p2pPending.contains(src) && !p2pAccepted.contains(src))
                        {
                            p2pPending.removeAll(src);
                            p2pAccepted.append(src);

                            dataToClient(cmdId, data.mid(57), P2P_OPEN);
                        }
                    }
                    else if (data[56] == P2P_CLOSE)
                    {
                        if (p2pPending.contains(src))
                        {
                            p2pPending.removeAll(src);

                            dataToClient(cmdId, data.mid(57), P2P_CLOSE);
                        }
                        else if (p2pAccepted.contains(src))
                        {
                            p2pAccepted.removeAll(src);

                            dataToClient(cmdId, data.mid(57), P2P_CLOSE);
                        }
                    }
                    else if (p2pAccepted.contains(src))
                    {
                        dataToClient(cmdId, src + data.mid(57), static_cast<uchar>(data[56]));
                    }
                }
            }
            else if (cmdId == ASYNC_CLOSE_P2P)
            {
                // format: [28bytes(src_sessionId)]

                if (p2pAccepted.contains(data) || p2pPending.contains(data))
                {
                     p2pAccepted.removeAll(data);
                     p2pPending.removeAll(data);

                     dataToClient(ASYNC_P2P, data, P2P_CLOSE);
                }
            }
            else if (cmdId == ASYNC_LIMITED_CAST)
            {
                // format: [54bytes(chIds)][1byte(typeId)][rest-of-bytes(payload)]

                if (activeUpdate && matchChs(chIds, QByteArray::fromRawData(data.data(), 54)))
                {
                    if (data[54] == PING_PEERS)
                    {
                        // PING_PEERS is formatted exactly like PEER_INFO. it only tells this
                        // async command to also send PEER_INFO of this session to the session
                        // that requested the ping using ASYNC_TO_PEER.

                        QByteArray peerId = data.mid(55, 28);
                        QByteArray typeId = wrInt(PEER_INFO, 8);
                        QByteArray info   = toPEER_INFO(shared);

                        dataToClient(ASYNC_TO_PEER, peerId + typeId + info, PUB_IPC);
                        dataToClient(cmdId, data.mid(55), PEER_INFO);
                    }
                    else
                    {
                        dataToClient(cmdId, data.mid(55), static_cast<uchar>(data[54]));
                    }
                }
            }
            else if ((cmdId == ASYNC_GROUP_RENAMED) || (cmdId == ASYNC_GRP_TRANS))
            {
                QStringList args = parseArgs(data, -1);
                QString     name = getParam("-src", args);

                if (noCaseMatch(groupName, name))
                {
                    groupName = getParam("-dst", args);

                    if (cmdId == ASYNC_GRP_TRANS)
                    {
                        hostRank = getRankForGroup(groupName);

                        emit loadCommands();

                        sendLocalInfo();
                    }
                }
            }
            else if (cmdId == ASYNC_RW_MY_INFO)
            {
                if (noCaseMatch(userName, fromTEXT(data)))
                {
                    sendLocalInfo();
                }
            }
            else if (cmdId == ASYNC_USER_RENAMED)
            {
                QStringList args = parseArgs(data, -1);
                QString     name = getParam("-old", args);

                if (noCaseMatch(userName, name))
                {
                    userName = getParam("-new_name", args);

                    castPeerInfo();
                    sendLocalInfo();
                }
            }
            else if (cmdId == ASYNC_DISP_RENAMED)
            {
                QStringList args  = parseArgs(data, -1);
                QString     uName = getParam("-user", args);

                if (noCaseMatch(userName, uName))
                {
                    displayName = getParam("-name", args);

                    castPeerInfo();
                    sendLocalInfo();
                }
            }
            else if (cmdId == ASYNC_USER_GROUP_CHANGED)
            {
                QStringList args  = parseArgs(data, -1);
                QString     uName = getParam("-user", args);

                if (noCaseMatch(userName, uName))
                {
                    groupName = getParam("-group", args);
                    hostRank  = getRankForGroup(groupName);

                    emit loadCommands();

                    sendLocalInfo();
                }
            }
            else if (cmdId == ASYNC_CMD_RANKS_CHANGED)
            {
                emit loadCommands();
            }
            else if (cmdId == ASYNC_GROUP_UPDATED)
            {
                QStringList args = parseArgs(data, -1);
                QString     name = getParam("-name", args);

                if (noCaseMatch(groupName, name))
                {
                    hostRank = getParam("-rank", args).toUInt();

                    emit loadCommands();
                }
            }
            else if (cmdId == ASYNC_RESTORE_AUTH)
            {
                Query db(this);

                db.setType(Query::PULL, TABLE_USERS);
                db.addColumn(COLUMN_USERNAME);
                db.addColumn(COLUMN_GRNAME);
                db.addColumn(COLUMN_DISPLAY_NAME);
                db.addCondition(COLUMN_USER_ID, data);
                db.exec();

                userId      = data;
                userName    = db.getData(COLUMN_USERNAME).toString();
                groupName   = db.getData(COLUMN_GRNAME).toString();
                displayName = db.getData(COLUMN_DISPLAY_NAME).toString();
                hostRank    = getRankForGroup(groupName);

                authOk();
                dataToClient(ASYNC_RDY, toTEXT("\nReady!\n\n"), TEXT);
            }
            else if (cmdId == ASYNC_PUBLIC_AUTH)
            {
                userName.clear();
                groupName.clear();
                displayName.clear();
                userId.clear();
                chIds.clear();
                chList.clear();

                hostRank = 0;

                authOk();
                dataToClient(ASYNC_RDY, toTEXT("\nReady!\n\n"), TEXT);
            }
            else if (cmdId == ASYNC_ENABLE_MOD)
            {
                emit loadModFile(fromTEXT(data));
                emit loadCommands();
            }
            else if (cmdId == ASYNC_DISABLE_MOD)
            {
                emit unloadModFile(fromTEXT(data));
            }
            else if ((cmdId == ASYNC_NEW_CH_MEMBER) || (cmdId == ASYNC_INVITED_TO_CH) ||
                     (cmdId == ASYNC_INVITE_ACCEPTED))
            {
                QStringList args   = parseArgs(data, -1);
                QString     user   = getParam("-user", args);
                QString     chName = getParam("-ch_name", args).toLower();

                if (noCaseMatch(user, userName))
                {
                    if ((cmdId == ASYNC_NEW_CH_MEMBER) || (cmdId == ASYNC_INVITE_ACCEPTED))
                    {
                        chList.append(chName);
                    }

                    dataToClient(cmdId, data, TEXT);
                }
            }
            else if (cmdId == ASYNC_DEL_CH)
            {
                QStringList args   = parseArgs(data, -1);
                QString     chName = getParam("-ch_name", args).toLower();

                if (chList.contains(chName))
                {
                    chList.removeAll(chName);

                    wrCloseCh(rwShared, getParam("-ch_id", args).toULongLong());
                    dataToClient(cmdId, data, TEXT);
                }
            }
            else if (cmdId == ASYNC_RM_CH_MEMBER)
            {
                QStringList args  = parseArgs(data, -1);
                QString     uName = getParam("-user", args);

                if (noCaseMatch(userName, uName))
                {
                    QString chName = getParam("-ch_name", args).toLower();

                    chList.removeAll(chName);

                    QByteArray peerStat;

                    wrCloseCh(rwShared, getParam("-ch_id", args).toULongLong(), peerStat);

                    if (!peerStat.isEmpty())
                    {
                        dataToClient(ASYNC_LIMITED_CAST, peerStat, PUB_IPC);
                    }

                    dataToClient(cmdId, data, TEXT);
                }
            }
            else if (cmdId == ASYNC_MEM_LEVEL_CHANGED)
            {
                QStringList args   = parseArgs(data, -1);
                QString     uName  = getParam("-user", args);
                QString     chName = getParam("-ch_name", args).toLower();

                if (noCaseMatch(userName, uName))
                {
                    QByteArray peerStat;

                    wrCloseCh(rwShared, getParam("-ch_id", args).toULongLong(), peerStat);

                    if (!peerStat.isEmpty())
                    {
                        dataToClient(ASYNC_LIMITED_CAST, peerStat, PUB_IPC);
                    }

                    dataToClient(cmdId, data, TEXT);
                }
                else if (chList.contains(chName))
                {
                    dataToClient(cmdId, data, TEXT);
                }
            }
            else if (cmdId == ASYNC_RENAME_CH)
            {
                QStringList args   = parseArgs(data, -1);
                QString     chName = getParam("-ch_name", args).toLower();

                if (chList.contains(chName))
                {
                    QString newName = getParam("-new_name", args).toLower();

                    chList.removeAll(chName);
                    chList.append(newName);

                    dataToClient(cmdId, data, TEXT);
                }
            }
            else if (cmdId == ASYNC_CH_ACT_FLAG)
            {
                QStringList args   = parseArgs(data, -1);
                QString     chName = getParam("-ch_name", args).toLower();

                if (chList.contains(chName))
                {
                    activeUpdate = containsActiveCh(chIds);
                }
            }
            else if ((cmdId == ASYNC_NEW_SUB_CH) || (cmdId == ASYNC_RENAME_SUB_CH))
            {
                QStringList args   = parseArgs(data, -1);
                QString     chName = getParam("-ch_name", args).toLower();

                if (chList.contains(chName))
                {
                    dataToClient(cmdId, data, TEXT);
                }
            }
            else if ((cmdId == ASYNC_RM_SUB_CH) || (cmdId == ASYNC_SUB_CH_LEVEL_CHG) ||
                     (cmdId == ASYNC_RM_RDONLY) || (cmdId == ASYNC_ADD_RDONLY))
            {
                QStringList args   = parseArgs(data, -1);
                QString     chName = getParam("-ch_name", args).toLower();
                QString     chId   = getParam("-ch_id", args);
                QString     subId  = getParam("-sub_id", args);
                QByteArray  chSub  = wrInt(chId.toULongLong(), 64) + wrInt(subId.toUInt(), 8);

                int pos = chPos(chSub, chIds);

                if (pos != -1)
                {
                    wrCloseCh(rwShared, chSub);
                    dataToClient(cmdId, data, TEXT);
                }
                else if (chList.contains(chName))
                {
                    dataToClient(cmdId, data, TEXT);
                }
            }
        }
    }
}

void Session::sendStdout()
{
    if (isSlave())
    {
        qDebug() << "CmdExecutor: " << slaveProc->readAllStandardOutput();
    }
}

void Session::sendStderr()
{
    if (isSlave())
    {
        qDebug() << "CmdExecutor Err: " << slaveProc->readAllStandardError();
    }
}
