#include "cmd_proc.h"

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

ModProcess::ModProcess(const QString &app, const QString &memSes, const QString &memHos, const QString &pipe, QObject *parent) : QProcess(parent)
{
    flags          = 0;
    ipcTypeId      = 0;
    ipcDataSize    = 0;
    hostRank       = 0;
    modCmdNames    = nullptr;
    cmdUniqueNames = nullptr;
    cmdRealNames   = nullptr;
    cmdAppById     = nullptr;
    cmdIds         = nullptr;
    ipcSocket      = nullptr;
    ipcServ        = new QLocalServer(this);
    idleTimer      = new IdleTimer(this);
    sesMemKey      = memSes;
    hostMemKey     = memHos;
    pipeName       = pipe;

    ipcServ->setMaxPendingConnections(1);

    connect(this, &QProcess::readyReadStandardError, this, &ModProcess::rdFromStdErr);
    connect(this, &QProcess::readyReadStandardOutput, this, &ModProcess::rdFromStdOut);
    connect(this, &QProcess::errorOccurred, this, &ModProcess::err);
    connect(this, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(onFinished(int,QProcess::ExitStatus)));

    connect(ipcServ, &QLocalServer::newConnection, this, &ModProcess::newIPCLink);
    connect(idleTimer, &IdleTimer::timeout, this, &ModProcess::killProc);

    setProgram(app);
}

void ModProcess::rdFromStdErr()
{
    emit dataToClient(toCmdId32(ASYNC_SYS_MSG, 0), toTEXT(readAllStandardError()), ERR);
}

void ModProcess::rdFromStdOut()
{
    emit dataToClient(toCmdId32(ASYNC_SYS_MSG, 0), toTEXT(readAllStandardOutput()), TEXT);
}

quint16 ModProcess::genCmdId()
{
    quint16 ret = 256;

    if (flags & SESSION_PARAMS_SET)
    {
        while(cmdIds->contains(ret)) ret++;
    }

    return ret;
}

QString ModProcess::makeCmdUnique(const QString &name)
{
    QString     strNum;
    QStringList names = cmdUniqueNames->values();

    for (int i = 1; names.contains(name + strNum); ++i)
    {
        strNum = "_" + QString::number(i);
    }

    return QString(name + strNum).toLower();
}

bool ModProcess::isCmdLoaded(const QString &name)
{
    bool ret = false;

    if (modCmdNames->contains(program()))
    {
        if (modCmdNames->value(program()).contains(name))
        {
            ret = true;
        }
    }

    return ret;
}

bool ModProcess::allowCmdLoad(const QString &cmdName)
{
    bool ret = false;

    if (validCommandName(cmdName))
    {
        if (flags & (LOADING_PUB_CMDS | LOADING_EXEMPT_CMDS))
        {
            ret = true;
        }
        else if (!cmdRanks.contains(cmdName))
        {
            ret = (hostRank == 1);
        }
        else
        {
            ret = (cmdRanks[cmdName] >= hostRank);
        }
    }

    return ret;
}

void ModProcess::onDataFromProc(quint8 typeId, const QByteArray &data)
{
    if ((typeId == NEW_CMD) && (flags & SESSION_PARAMS_SET))
    {
        if (data.size() >= 259)
        {
            // a valid NEW_CMD must have a minimum of 259 bytes.

            QString cmdName = fromTEXT(data.mid(3, 128)).trimmed().toLower();

             if (isCmdLoaded(cmdName))
             {
                 if (!allowCmdLoad(cmdName))
                 {
                     quint16 cmdId = cmdRealNames->key(cmdName);

                     cmdIds->removeOne(cmdId);
                     cmdRealNames->remove(cmdId);
                     cmdUniqueNames->remove(cmdId);
                     cmdAppById->remove(cmdId);

                     if (modCmdNames->contains(program()))
                     {
                         modCmdNames->operator[](program()).removeOne(cmdName);
                     }

                     emit cmdUnloaded(cmdId);
                     emit dataToClient(toCmdId32(ASYNC_RM_CMD, 0), wrInt(cmdId, 16), CMD_ID);
                 }
             }
             else if (allowCmdLoad(cmdName))
             {
                 quint16    cmdId   = genCmdId();
                 QByteArray cmdIdBa = wrInt(cmdId, 16);
                 QString    unique  = makeCmdUnique(cmdName);

                 cmdIds->append(cmdId);
                 cmdRealNames->insert(cmdId, cmdName);
                 cmdUniqueNames->insert(cmdId, unique);
                 cmdAppById->insert(cmdId, program());

                 if (modCmdNames->contains(program()))
                 {
                     modCmdNames->operator[](program()).append(cmdName);
                 }
                 else
                 {
                     QStringList list = QStringList() << cmdName;

                     modCmdNames->insert(program(), list);
                 }

                 QByteArray frame = cmdIdBa + data.mid(2, 1) + fixedToTEXT(unique, 128) + data.mid(131);

                 emit dataToClient(toCmdId32(ASYNC_ADD_CMD, 0), frame, NEW_CMD);
             }
        }
    }
    else if (typeId == ERR)
    {
        qDebug() << fromTEXT(data);
    }
}

void ModProcess::rdFromIPC()
{
    if (flags & FRAME_RDY)
    {
        if (ipcSocket->bytesAvailable() >= ipcDataSize)
        {
            onDataFromProc(ipcTypeId, ipcSocket->read(ipcDataSize));

            flags ^= FRAME_RDY;

            rdFromIPC();
        }
    }
    else if (ipcSocket->bytesAvailable() >= (FRAME_HEADER_SIZE - 4))
    {
        QByteArray header = ipcSocket->read(FRAME_HEADER_SIZE - 4);

        ipcTypeId   = static_cast<quint8>(header[0]);
        ipcDataSize = static_cast<quint32>(rdInt(header.mid(1, 3)));
        flags      |= FRAME_RDY;

        rdFromIPC();
    }
}

void ModProcess::ipcDisconnected()
{
    if (ipcSocket != nullptr)
    {
        ipcSocket->deleteLater();
    }

    ipcSocket = nullptr;
}

void ModProcess::newIPCLink()
{
    if (ipcSocket != nullptr)
    {
        ipcServ->nextPendingConnection()->deleteLater();
    }
    else
    {
        ipcSocket = ipcServ->nextPendingConnection();

        connect(ipcSocket, &QLocalSocket::readyRead, this, &ModProcess::rdFromIPC);
        connect(ipcSocket, &QLocalSocket::disconnected, this, &ModProcess::ipcDisconnected);

        idleTimer->attach(ipcSocket, 120000); //2min idle timeout

        onReady();
    }
}

void ModProcess::setSessionParams(QHash<quint16, QString> *uniqueNames,
                                  QHash<quint16, QString> *realNames,
                                  QHash<quint16, QString> *appById,
                                  QHash<QString, QStringList> *namesForMod,
                                  QList<quint16> *ids,
                                  quint32 rnk)
{
    flags         |= SESSION_PARAMS_SET;
    modCmdNames    = namesForMod;
    cmdUniqueNames = uniqueNames;
    cmdRealNames   = realNames;
    cmdAppById     = appById;
    cmdIds         = ids;
    hostRank       = rnk;

    Query db(this);

    db.setType(Query::PULL, TABLE_CMD_RANKS);
    db.addColumn(COLUMN_HOST_RANK);
    db.addColumn(COLUMN_COMMAND);
    db.addCondition(COLUMN_MOD_MAIN, program());
    db.exec();

    for (int i = 0; i < db.rows(); ++i)
    {
        cmdRanks.insert(db.getData(COLUMN_COMMAND, i).toString(), db.getData(COLUMN_HOST_RANK, i).toUInt());
    }
}

void ModProcess::onFailToStart()
{
    emit dataToClient(toCmdId32(ASYNC_SYS_MSG, 0), toTEXT("\nerr: A module failed to start so some commands may not have loaded. detailed error information was logged for admin review.\n"), ERR);
}

void ModProcess::err(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart)
    {
        qDebug() << "err: Module process: " << program() << " failed to start. reason: " << errorString();

        emit finished(1, QProcess::CrashExit);

        onFailToStart();
    }
}

bool ModProcess::openPipe()
{
    bool ret = ipcServ->listen(pipeName);

    fullPipe = ipcServ->fullServerName();

    if (!ipcServ->isListening())
    {
        QFile::remove(fullPipe);

        ret = ipcServ->listen(pipeName);
    }

    return ret;
}

bool ModProcess::startProc(const QStringList &args)
{
    bool ret = false;

    if (openPipe())
    {
        fullPipe = ipcServ->fullServerName();

        setArguments(QStringList() << "-pipe_name" << fullPipe << "-mem_ses" << sesMemKey << "-mem_host" << hostMemKey << args);
        start();
    }
    else
    {
        setErrorString("Unable to open pipe: " + fullPipe + " " + ipcServ->errorString());

        emit errorOccurred(QProcess::FailedToStart);
    }

    return ret;
}

bool ModProcess::loadPublicCmds()
{
    flags |= LOADING_PUB_CMDS;

    return startProc(QStringList() << "-public_cmds");
}

bool ModProcess::loadUserCmds()
{
    flags |= LOADING_USER_CMDS;

    return startProc(QStringList() << "-user_cmds");
}

bool ModProcess::loadExemptCmds()
{
    flags |= LOADING_EXEMPT_CMDS;

    return startProc(QStringList() << "-exempt_cmds");
}

void ModProcess::cleanupPipe()
{
    ipcServ->close();

    if (QFile::exists(fullPipe))
    {
        QFile::remove(fullPipe);
    }

    ipcDisconnected();
}

void ModProcess::onReady()
{
    QStringList hostVer = QCoreApplication::applicationVersion().split('.');
    QByteArray  verFrame;

    verFrame.append(wrInt(hostVer[0].toULongLong(), 16));
    verFrame.append(wrInt(hostVer[1].toULongLong(), 16));
    verFrame.append(wrInt(hostVer[2].toULongLong(), 16));

    wrIpcFrame(HOST_VER, verFrame);
}

void ModProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)

    cleanupPipe();
    deleteLater();
}

void ModProcess::wrIpcFrame(quint8 typeId, const QByteArray &data)
{
    if (ipcSocket != nullptr)
    {
        ipcSocket->write(wrInt(typeId, 8) + wrInt(data.size(), MAX_FRAME_BITS) + data);
    }
}

void ModProcess::killProc()
{
    wrIpcFrame(KILL_CMD, QByteArray());

    QTimer::singleShot(3000, this, SLOT(kill()));
}

CmdProcess::CmdProcess(quint32 id, const QString &cmd, const QString &modApp, const QString &memSes, const QString &memHos, const QString &pipe, QObject *parent) : ModProcess(modApp, memSes, memHos, pipe, parent)
{
    cmdId   = id;
    cmdName = cmd;
    cmdIdle = false;
}

void CmdProcess::setSessionParams(QSharedMemory *mem, char *sesId, char *wrableSubChs)
{
    sesMem             = mem;
    sessionId          = sesId;
    openWritableSubChs = wrableSubChs;
}

void CmdProcess::killCmd16(quint16 id16)
{
    if (toCmdId16(cmdId) == id16)
    {
        killProc();
    }
}

void CmdProcess::killCmd32(quint32 id32)
{
    if (cmdId == id32)
    {
        killProc();
    }
}

void CmdProcess::onReady()
{
    emit cmdProcReady(cmdId, this);
}

void CmdProcess::onFailToStart()
{
    emit dataToClient(cmdId, toTEXT("err: The command failed to start. error details were logged for admin review.\n"), ERR);
    emit dataToClient(cmdId, wrInt(FAILED_TO_START, 16), IDLE);
}

void CmdProcess::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!cmdIdle)
    {
        emit dataToClient(cmdId, toTEXT("err: The command has stopped unexpectedly or it has failed to send an IDLE frame before exiting.\n"), ERR);
        emit dataToClient(cmdId, wrInt(CRASH, 16), IDLE);
    }

    emit cmdProcFinished(cmdId);

    ModProcess::onFinished(exitCode, exitStatus);
}

void CmdProcess::rdFromStdErr()
{
    emit dataToClient(cmdId, toTEXT(readAllStandardError()), ERR);
}

void CmdProcess::rdFromStdOut()
{
    emit dataToClient(cmdId, toTEXT(readAllStandardOutput()), TEXT);
}

void CmdProcess::dataFromSession(quint32 id, const QByteArray &data, quint8 dType)
{
    if (id == cmdId)
    {
        cmdIdle = false;

        wrIpcFrame(dType, data);
    }
}

bool CmdProcess::validAsync(quint16 async, const QByteArray &data, QTextStream &errMsg)
{
    bool ret = true;

    if ((async == ASYNC_USER_DELETED) || (async == ASYNC_RW_MY_INFO) || (async == ASYNC_USER_LOGIN))
    {
        if (data.size() != BLKSIZE_USER_ID)
        {
            ret = false; errMsg << "the 256bit user id is not " << BLKSIZE_USER_ID << " bytes long.";
        }
    }
    else if (async == ASYNC_USER_RENAMED)
    {
        if (data.size() != (BLKSIZE_USER_ID + BLKSIZE_USER_NAME))
        {
            ret = false; errMsg << "expected data containing the user id and name to be " << (BLKSIZE_USER_ID + BLKSIZE_USER_NAME) << " bytes long.";
        }
    }
    else if (async == ASYNC_DISP_RENAMED)
    {
        if (data.size() != (BLKSIZE_USER_ID + BLKSIZE_DISP_NAME))
        {
            ret = false; errMsg << "expected data containing the user id and display name to be " << (BLKSIZE_USER_ID + BLKSIZE_DISP_NAME) << " bytes long.";
        }
    }
    else if (async == ASYNC_MAXSES)
    {
        if (data.size() != BLKSIZE_HOST_LOAD)
        {
            ret = false; errMsg << "the 32bit max session int is not " << BLKSIZE_HOST_LOAD << " bytes long.";
        }
    }
    else if (async == ASYNC_USER_RANK_CHANGED)
    {
        if (data.size() != (BLKSIZE_USER_ID + BLKSIZE_HOST_RANK))
        {
            ret = false; errMsg << "expected data containing the user id and host rank to be " << (BLKSIZE_USER_ID + BLKSIZE_HOST_RANK) << " bytes long.";
        }
    }
    else if ((async == ASYNC_CAST) || (async == ASYNC_LIMITED_CAST))
    {
        int payloadOffs = (MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL) + 1;

        sesMem->lock();

        if (data.size() < payloadOffs)
        {
            ret = false; errMsg << "cast header is not at least " << payloadOffs << " bytes long.";
        }
        else if (memcmp(data.data(), openWritableSubChs, static_cast<quint32>(payloadOffs - 1)) != 0)
        {
            ret = false; errMsg << "the open sub-channels (writable) does not match the actual open sub-channels.";
        }
        else if (rd8BitFromBlock(data.data() + (payloadOffs - 1)) == PING_PEERS)
        {
            // casting PING_PEERS to active update sessions are blocked.
            // command processes should use ASYNC_PING_PEERS instead.

            ret = false; errMsg << "attempted to cast PING_PEERS which is forbidden for module commands.";
        }

        sesMem->unlock();
    }
    else if (async == ASYNC_TO_PEER)
    {
        ret = false; errMsg << "ASYNC_TO_PEER is forbidden for module commands.";
    }
    else if (async == ASYNC_P2P)
    {
        int payloadOffs = ((BLKSIZE_SESSION_ID * 2) + 1);

        if (data.size() < payloadOffs)
        {
            ret = false; errMsg << "p2p header is not at least " << payloadOffs << " bytes long.";
        }
        else if (memcmp(data.data() + BLKSIZE_SESSION_ID, sessionId, BLKSIZE_SESSION_ID) != 0)
        {
            // make sure P2P async commands source session id is the actual the local
            // session. fraudulent P2P async's are blocked.

            ret = false; errMsg << "the source session id does not match the actual session id.";
        }
    }
    else if (async == ASYNC_CLOSE_P2P)
    {
        if (data.size() < BLKSIZE_SESSION_ID)
        {
            ret = false; errMsg << "p2p header is not at least " << BLKSIZE_SESSION_ID << " bytes long.";
        }
        else if (memcmp(data.data(), sessionId, BLKSIZE_SESSION_ID) != 0)
        {
            // make sure P2P async commands source session id is the actual the local
            // session. fraudulent P2P async's are blocked.

            ret = false; errMsg << "the source session id does not match the actual session id.";
        }
    }
    else if ((async == ASYNC_OPEN_SUBCH) || (async == ASYNC_CLOSE_SUBCH))
    {
        if (data.size() != BLKSIZE_SUB_CHANNEL)
        {
            ret = false; errMsg << "the 72bit sub-channel id is not " << BLKSIZE_SUB_CHANNEL << " bytes long.";
        }
    }
    else if ((async == ASYNC_RM_SUB_CH)   || (async == ASYNC_SUB_CH_LEVEL_CHG) ||
             (async == ASYNC_RM_RDONLY)   || (async == ASYNC_ADD_RDONLY)       ||
             (async == ASYNC_CH_ACT_FLAG) || (async == ASYNC_NEW_SUB_CH)       ||
             (async == ASYNC_RENAME_SUB_CH))
    {
        if (data.size() < BLKSIZE_SUB_CHANNEL)
        {
            ret = false; errMsg << "a 72bit sub-channel id header is not present.";
        }
    }
    else if ((cmdId == ASYNC_NEW_CH_MEMBER)   || (cmdId == ASYNC_INVITED_TO_CH) ||
             (cmdId == ASYNC_INVITE_ACCEPTED) || (cmdId == ASYNC_RM_CH_MEMBER) ||
             (cmdId == ASYNC_MEM_LEVEL_CHANGED))
    {
        if (data.size() < (BLKSIZE_USER_ID + BLKSIZE_CHANNEL_ID))
        {
            ret = false; errMsg << "the channel member info header is not at least " << (BLKSIZE_USER_ID + BLKSIZE_CHANNEL_ID) << "bytes long.";
        }
    }
    else if ((cmdId == ASYNC_RENAME_CH) || (cmdId == ASYNC_DEL_CH))
    {
        if (data.size() < BLKSIZE_CHANNEL_ID)
        {
            ret = false; errMsg << "a 64bit channel id header was not found.";
        }
    }

    return ret;
}

void CmdProcess::onDataFromProc(quint8 typeId, const QByteArray &data)
{
    if ((typeId == PRIV_IPC) || (typeId == PUB_IPC) || (typeId == PUB_IPC_WITH_FEEDBACK))
    {
        if (data.size() >= 2)
        {
            quint16 async = rd16BitFromBlock(data.data());

            // ASYNC_KEEP_ALIVE is blocked but not considered an error. it has already done
            // it's job by getting transffered so it doesn't need to go any further.

            if (async != ASYNC_KEEP_ALIVE)
            {
                QByteArray payload = rdFromBlock(data.data() + 2, static_cast<quint32>(data.size() - 2));

                QString     errMsg;
                QTextStream errTxt(&errMsg);

                if (async == ASYNC_DEBUG_TEXT)
                {
                    typeId = PRIV_IPC;
                }

                if (validAsync(async, payload, errTxt))
                {
                    if (typeId == PRIV_IPC)
                    {
                        emit privIPC(async, payload);
                    }
                    else if (typeId == PUB_IPC)
                    {
                        emit pubIPC(async, payload);
                    }
                    else
                    {
                        emit pubIPCWithFeedBack(async, payload);
                    }
                }
                else
                {
                    qDebug() << "async id: " << async << " from command id: " << toCmdId16(cmdId) << " blocked. reason: " << errMsg;
                }
            }
        }
    }
    else
    {
        if (typeId == IDLE)
        {
            cmdIdle = true;

            if (data.isEmpty())
            {
                emit dataToClient(cmdId, wrInt(NO_ERRORS, 16), typeId);
            }
            else
            {
                emit dataToClient(cmdId, data, typeId);
            }
        }
        else
        {
            emit dataToClient(cmdId, data, typeId);
        }
    }
}

bool CmdProcess::startCmdProc()
{
    return startProc(QStringList() << "-run_cmd" << cmdName);
}
