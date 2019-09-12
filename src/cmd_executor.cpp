#include "cmd_executor.h"

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

CmdExecutor::CmdExecutor(RWSharedObjs *rwShare, SharedObjs *rdOnlyShare, QSharedMemory *debugInfo, QObject *parent) : QObject(parent)
{
    rwShare->commands       = &commands;
    rwShare->activeLoopCmds = &activeLoopCmds;
    rwShare->pausedCmds     = &pausedCmds;
    rwShare->moreInputCmds  = &moreInputCmds;
    rwShare->cmdNames       = &cmdNames;

    rdOnlyShare->activeLoopCmds = &activeLoopCmds;
    rdOnlyShare->pausedCmds     = &pausedCmds;
    rdOnlyShare->moreInputCmds  = &moreInputCmds;
    rdOnlyShare->cmdNames       = &cmdNames;

    loopIndex    = 0;
    exeDebugInfo = debugInfo;
    rdSharedObjs = rdOnlyShare;
    rwSharedObjs = rwShare;
    internalCmds = nullptr;

    connect(this, &CmdExecutor::loop, this, &CmdExecutor::exeCmd);
}

void CmdExecutor::buildCmdLoaders()
{
    internalCmds = new InternalCommandLoader(rwSharedObjs, this);

    cmdLoaders.insert(INTERN_MOD_NAME, internalCmds);

    Query db(this);

    db.setType(Query::PULL, TABLE_MODULES);
    db.addColumn(COLUMN_MOD_NAME);
    db.addCondition(COLUMN_LOCKED, false);
    db.exec();

    for (int i = 0; i < db.rows(); ++i)
    {
        loadModFile(db.getData(COLUMN_MOD_NAME, i).toString());
    }
}

void CmdExecutor::wrCrashDebugInfo(const QString &msg)
{
    if (exeDebugInfo->isAttached())
    {
        exeDebugInfo->lock();

        QByteArray data = toTEXT(msg).leftJustified(EXE_DEBUG_INFO_SIZE, static_cast<char>(0), true);

        memcpy(exeDebugInfo->data(), data.data(), EXE_DEBUG_INFO_SIZE);

        exeDebugInfo->unlock();
    }
}

void CmdExecutor::connectExternCmd(ExternCommand *cmd, quint16 cmdId, const QString &cmdName)
{
    wrCrashDebugInfo(" exe func: connectExternCmd()\n cmd id: " + QString::number(cmdId) + "\n cmd name: " + cmdName);

    auto *cmdOutput = new CommandOutput(cmd);

    connect(cmdOutput, &CommandOutput::dataOut, this, &CmdExecutor::externDataToIPC);

    connectCommon(cmd, cmdOutput, cmdId, cmdName);
}

void CmdExecutor::connectInternCmd(InternCommand *cmd, quint16 cmdId, const QString &cmdName)
{
    wrCrashDebugInfo(" exe func: connectInternCmd()\n cmd id: " + QString::number(cmdId) + "\n cmd name: " + cmdName);

    auto *cmdOutput = new CommandOutput(cmd);

    connect(cmdOutput, &CommandOutput::dataOut, this, &CmdExecutor::internDataToIPC);

    connect(cmd, &InternCommand::backendDataOut, this, &CmdExecutor::backendFromCmd);
    connect(cmd, &InternCommand::authOk, this, &CmdExecutor::authOk);
    connect(cmd, &InternCommand::termAllCommands, this, &CmdExecutor::termAllCommands);
    connect(cmd, &InternCommand::termCommandId, this, &CmdExecutor::termCommandId);
    connect(cmd, &InternCommand::reloadCommands, this, &CmdExecutor::buildCommands);

    connectCommon(cmd, cmdOutput, cmdId, cmdName);
}

void CmdExecutor::connectCommon(ExternCommand *cmd, CommandOutput *cmdOutput, quint16 cmdId, const QString &cmdName)
{
    wrCrashDebugInfo(" exe func: connectCommon()\n cmd id: " + QString::number(cmdId) + "\n cmd name: " + cmdName);

    connect(cmdOutput, &CommandOutput::closeChById, this, &CmdExecutor::closeChById);
    connect(cmdOutput, &CommandOutput::openChById, this, &CmdExecutor::openChById);
    connect(cmdOutput, &CommandOutput::closeChByName, this, &CmdExecutor::closeChByName);
    connect(cmdOutput, &CommandOutput::openChByName, this, &CmdExecutor::openChByName);
    connect(cmdOutput, &CommandOutput::cmdFinished, this, &CmdExecutor::commandFinished);
    connect(cmdOutput, &CommandOutput::enableLoop, this, &CmdExecutor::enableLoop);
    connect(cmdOutput, &CommandOutput::enableMoreInput, this, &CmdExecutor::enableMoreInput);

    connect(cmd, &ExternCommand::closeChById, cmdOutput, &CommandOutput::closeChIdFromCmdObj);
    connect(cmd, &ExternCommand::openChById, cmdOutput, &CommandOutput::openChIdFromCmdObj);
    connect(cmd, &ExternCommand::closeChByName, cmdOutput, &CommandOutput::closeChNameFromCmdObj);
    connect(cmd, &ExternCommand::openChByName, cmdOutput, &CommandOutput::openChNameFromCmdObj);
    connect(cmd, &ExternCommand::cmdFinished, cmdOutput, &CommandOutput::finished);
    connect(cmd, &ExternCommand::enableLoop, cmdOutput, &CommandOutput::enableLoopFromCmdObj);
    connect(cmd, &ExternCommand::enableMoreInput, cmdOutput, &CommandOutput::enableMoreInputFromCmdObj);
    connect(cmd, &ExternCommand::dataToClient, cmdOutput, &CommandOutput::dataFromCmdObj);

    connect(cmd, &ExternCommand::castToPeers, this, &CmdExecutor::castToPeers);
    connect(cmd, &ExternCommand::toPeer, this, &CmdExecutor::toPeer);
    connect(cmd, &ExternCommand::closeSession, this, &CmdExecutor::endSession);
    connect(cmd, &ExternCommand::logout, this, &CmdExecutor::logout);

    cmd->cmdId           = cmdId;
    cmd->inMoreInputMode = false;
    cmd->inLoopMode      = false;
    cmd->errSent         = false;

    cmd->setObjectName(cmdName);
    cmdOutput->setCmdId(cmdId);
}

void CmdExecutor::enableLoop(quint16 cmdId, bool state)
{
    if (state)
    {
        uniqueAdd(cmdId, activeLoopCmds);
    }
    else
    {
        activeLoopCmds.removeAll(cmdId);
    }
}

void CmdExecutor::enableMoreInput(quint16 cmdId, bool state)
{
    if (state)
    {
        uniqueAdd(cmdId, moreInputCmds);
    }
    else
    {
        moreInputCmds.removeAll(cmdId);
    }
}

void CmdExecutor::preExe(ExternCommand *cmdObj, quint16 cmdId)
{
    cmdObj->cmdId   = cmdId;
    cmdObj->errSent = false;
}

void CmdExecutor::preExe(const QList<ExternCommand *> &cmdObjs, quint16 cmdId)
{
    for (auto cmdObj : cmdObjs)
    {
        preExe(cmdObj, cmdId);
    }
}

void CmdExecutor::exeCmd(quint16 cmdId, const QByteArray &data, uchar typeId)
{
    wrCrashDebugInfo(" exe func: exeCmd()\n cmd id: " + QString::number(cmdId) + "\n type id: " + QString::number(typeId));

    if (!commands.contains(cmdId))
    {
        emit dataToSession(cmdId, toTEXT("err: The requested command id: '" + QString::number(cmdId) + "' does not exists.\n"), ERR);
        emit dataToSession(cmdId, QByteArray(), IDLE);
    }
    else if (!pausedCmds.contains(cmdId))
    {
        ExternCommand *cmdObj = commands[cmdId];

        preExe(cmdObj, cmdId);
        preExe(cmdObj->internCommands.values(), cmdId);

        cmdObj->procBin(rdSharedObjs, data, typeId);

        if (!activeLoopCmds.contains(cmdId) && !moreInputCmds.contains(cmdId))
        {
            emit cmdObj->cmdFinished();
        }

        nextLoopCmd();
    }
}

void CmdExecutor::nextLoopCmd()
{
    if (!activeLoopCmds.isEmpty())
    {
        if (loopIndex == activeLoopCmds.size())
        {
            loopIndex = 0;
        }

        wrCrashDebugInfo(" exe func: nextLoopCmd()\n loop index: " + QString::number(loopIndex));

        emit loop(activeLoopCmds[loopIndex++], QByteArray(), TEXT);
    }
}

void CmdExecutor::termCommandObj(quint16 cmdId, ExternCommand *cmd, bool del)
{
    wrCrashDebugInfo(" exe func: termCommandObj()\n cmd id: " + QString::number(cmdId));

    if (moreInputCmds.contains(cmdId) || activeLoopCmds.contains(cmdId))
    {
        cmd->term();

        cmd->inLoopMode      = false;
        cmd->inMoreInputMode = false;

        for (auto internObj : cmd->internCommands.values())
        {
            if (internObj->inLoopMode || internObj->inMoreInputMode)
            {
                internObj->term();

                internObj->inLoopMode      = false;
                internObj->inMoreInputMode = false;
            }
        }

        emit dataToSession(cmdId, QByteArray(), IDLE);
    }

    moreInputCmds.removeAll(cmdId);
    activeLoopCmds.removeAll(cmdId);
    pausedCmds.removeAll(cmdId);

    if (del)
    {
        wrCrashDebugInfo(" exe func: termCommandObj()\n cmd id: " + QString::number(cmdId) + "\n note: deleting intern commands");

        for (auto internObj : cmd->internCommands.values())
        {
            internObj->aboutToDelete();
            internObj->deleteLater();
        }

        wrCrashDebugInfo(" exe func: termCommandObj()\n cmd id: " + QString::number(cmdId) + "\n note: calling the command object's aboutToDelete()");

        cmd->aboutToDelete();
        cmd->deleteLater();

        commands.remove(cmdId);
        cmdNames.remove(cmdId);

        for (auto&& list : cmdIdsByModName.values())
        {
            if (list.contains(cmdId))
            {
                list.removeAll(cmdId);

                break;
            }
        }

        emit dataToSession(ASYNC_RM_CMD, wrInt(cmdId, 16), CMD_ID);
    }
}

void CmdExecutor::termCommandId(quint16 cmdId)
{
    if (commands.contains(cmdId))
    {
        termCommandObj(cmdId, commands[cmdId]);
    }
}

void CmdExecutor::termCommandsInList(const QList<quint16> &cmds, bool del)
{
    for (auto&& cmdId : cmds)
    {
        termCommandObj(cmdId, commands[cmdId], del);
    }
}

void CmdExecutor::termAllCommands()
{
    termCommandsInList(moreInputCmds, false);
    termCommandsInList(activeLoopCmds, false);
    termCommandsInList(pausedCmds, false);
}

void CmdExecutor::close()
{
    termCommandsInList(commands.keys(), true);

    for (auto cmdLoader : cmdLoaders.values())
    {
        cmdLoader->aboutToDelete();
    }

    for (auto plugin : plugins.values())
    {
        plugin->unload();
        plugin->deleteLater();
    }

    cleanupDbConnection();

    emit okToDelete();
}

void CmdExecutor::commandFinished(quint16 cmdId)
{
    emit dataToSession(cmdId, QByteArray(), IDLE);

    moreInputCmds.removeAll(cmdId);
    activeLoopCmds.removeAll(cmdId);
    pausedCmds.removeAll(cmdId);
}

QString CmdExecutor::makeCmdUnique(const QString &name)
{
    QString     strNum;
    QStringList names = cmdNames.values();

    for (int j = 1; names.contains(QString(name + strNum).toLower()); ++j)
    {
        strNum = "_" + QString::number(j);
    }

    return QString(name + strNum).toLower();
}

void CmdExecutor::procInternRequest(ExternCommand *cmd, quint16 cmdId, const QString &cmdName)
{
    wrCrashDebugInfo(" exe func: procInternRequest()\n cmd id: " + QString::number(cmdId));

    QStringList internCmdNames = internalCmds->cmdList();

    for (auto&& reqCmdName : cmd->internRequest())
    {
        if (internCmdNames.contains(reqCmdName, Qt::CaseInsensitive))
        {
            InternCommand *cmdObj = internalCmds->cmdObj(reqCmdName);

            if (cmdObj != nullptr)
            {
                connectInternCmd(cmdObj, cmdId, cmdName);

                cmdObj->setParent(cmd);
                cmd->internCommands.insert(reqCmdName, cmdObj);
            }
        }
    }
}

quint16 CmdExecutor::getModIdOffs(const QString &name)
{
    if (name == INTERN_MOD_NAME)
    {
        return MAX_CMDS_PER_MOD;
    }
    else
    {
        Query db(this);

        db.setType(Query::PULL, TABLE_MODULES);
        db.addColumn(COLUMN_CMD_ID_OFFS);
        db.addCondition(COLUMN_MOD_NAME, name);
        db.exec();

        return static_cast<quint16>(db.getData(COLUMN_CMD_ID_OFFS).toUInt());
    }
}

QString CmdExecutor::getModFile(const QString &modName)
{
    Query db(this);

    db.setType(Query::PULL, TABLE_MODULES);
    db.addColumn(COLUMN_MOD_MAIN);
    db.addCondition(COLUMN_MOD_NAME, modName);
    db.exec();

    return db.getData(COLUMN_MOD_MAIN).toString();
}

void CmdExecutor::loadModFile(const QString &modName)
{
    bool           modOk        = false;
    QString        path         = getModFile(modName);
    auto          *pluginLoader = new QPluginLoader(path);
    QObject       *cmdLoaderObj = pluginLoader->instance();
    CommandLoader *cmdLoader    = qobject_cast<CommandLoader*>(cmdLoaderObj);

    if (!pluginLoader->isLoaded())
    {
        qDebug() << "CmdExecutor::loadModFile() err: failed to load mod lib file: " << path << " reason: " << pluginLoader->errorString();
    }
    else if (!cmdLoaderObj)
    {
        qDebug() << "CmdExecutor::loadModFile() err: failed to load mod lib file: " << path << " reason: the root component object could not be instantiated.";
    }
    else if (!cmdLoader)
    {
        qDebug() << "CmdExecutor::loadModFile() err: failed to load mod lib file: " << path << " reason: the ModCommandLoader object could not be instantiated.";
    }
    else if (cmdLoader->rev() < IMPORT_REV)
    {
        qDebug() << "CmdExecutor::loadModFile() err: failed to load mod lib file: " << path << " reason: module import rev " << cmdLoader->rev() << " not compatible with host rev " << IMPORT_REV << ".";
    }
    else if (!cmdLoader->hostRevOk(IMPORT_REV))
    {
        qDebug() << "CmdExecutor::loadModFile() err: failed to load mod lib file: " << path << " the module rejected the host import rev. reason: " << cmdLoader->lastError() << ".";
    }
    else
    {
        modOk = true;

        wrCrashDebugInfo(" exe func: loadModFile()\n path: " + path + " \nmod name: " + modName + " \nnote: calling the module's modPath() function.");

        cmdLoader->modPath(QFileInfo(path).path());

        cmdLoaders.insert(modName, cmdLoader);
        plugins.insert(modName, pluginLoader);
    }

    if (!modOk)
    {
        pluginLoader->unload();
        pluginLoader->deleteLater();
    }
}

void CmdExecutor::unloadModFile(const QString &modName)
{
    if (cmdIdsByModName.contains(modName))
    {
        termCommandsInList(cmdIdsByModName[modName], true);

        wrCrashDebugInfo(" exe func: unloadModFile()\n mod name: " + modName + "\n note: calling the modules's aboutToDelete()");

        cmdLoaders[modName]->aboutToDelete();
        plugins[modName]->unload();
        plugins[modName]->deleteLater();

        cmdIdsByModName.remove(modName);
        cmdLoaders.remove(modName);
        plugins.remove(modName);
    }
}

void CmdExecutor::addCommandToList(quint16 cmdId, const QString &cmdName, const QString &modName, ExternCommand *cmdObj)
{
    procInternRequest(cmdObj, cmdId, cmdName);

    commands.insert(cmdId, cmdObj);
    cmdNames.insert(cmdId, cmdName);

    if (cmdIdsByModName.contains(modName))
    {
        cmdIdsByModName[modName].append(cmdId);
    }
    else
    {
        QList<quint16> list;

        list.append(cmdId);
        cmdIdsByModName.insert(modName, list);
    }

    emit dataToSession(ASYNC_ADD_CMD, toNEW_CMD(cmdId, cmdName, cmdObj), NEW_CMD);
}

bool CmdExecutor::allowCmdLoad(const QString &cmdName, const QString &modName, const QStringList &exemptList)
{
    bool ret = false;

    if (exemptList.contains(cmdName, Qt::CaseInsensitive))
    {
        ret = true;
    }
    else
    {
        Query db(this);

        db.setType(Query::PULL, TABLE_CMD_RANKS);
        db.addColumn(COLUMN_HOST_RANK);
        db.addCondition(COLUMN_COMMAND, cmdName);
        db.addCondition(COLUMN_MOD_NAME, modName);
        db.exec();

        if (db.rows())
        {
            uint cmdRank = db.getData(COLUMN_HOST_RANK).toUInt();

            if (cmdRank >= *rdSharedObjs->hostRank)
            {
                ret = true;
            }
        }
        else if (*rdSharedObjs->hostRank == 1)
        {
            ret = true;
        }
    }

    return ret;
}

void CmdExecutor::loadInternCmd(CommandLoader *loader, const QString &cmdName, const QString &uniqueName, quint16 id)
{
    auto *cmdObj = reinterpret_cast<InternCommand*>(loader->cmdObj(cmdName));

    if (cmdObj != nullptr)
    {
        connectInternCmd(cmdObj, id, uniqueName);
        addCommandToList(id, uniqueName, INTERN_MOD_NAME, cmdObj);
    }
}

void CmdExecutor::loadExternCmd(CommandLoader *loader, const QString &modName, const QString &cmdName, const QString &uniqueName, quint16 id)
{
    ExternCommand *cmdObj = loader->cmdObj(cmdName);

    if (cmdObj != nullptr)
    {
        connectExternCmd(cmdObj, id, uniqueName);
        addCommandToList(id, uniqueName, modName, cmdObj);
    }
}

void CmdExecutor::loadCmd(CommandLoader *loader, quint16 cmdId, const QString &modName, const QString &cmdName, const QString &uniqueCmdName)
{
    wrCrashDebugInfo(" exe func: loadCmd()\n cmd id: " + QString::number(cmdId) + "\n cmd name: " + cmdName);

    if (modName == INTERN_MOD_NAME)
    {
        loadInternCmd(loader, cmdName, uniqueCmdName, cmdId);
    }
    else
    {
        loadExternCmd(loader, modName, cmdName, uniqueCmdName, cmdId);
    }
}

void CmdExecutor::loadCmds(CommandLoader *loader, quint16 idOffs, const QString &modName)
{
    wrCrashDebugInfo(" exe func: loadCmds()\n mod name: " + modName + "\n id offs: " + QString::number(idOffs));

    QStringList list   = loader->cmdList();
    QStringList pub    = loader->pubCmdList();
    QStringList exempt = loader->rankExemptList();

    list.sort(Qt::CaseInsensitive);

    for (quint16 id = 0; (id < list.size()) && (id < MAX_CMDS_PER_MOD); ++id)
    {
        QString unique = makeCmdUnique(list[id]);
        quint16 cmdId  = idOffs + id;

        if (validCommandName(unique))
        {
            if (commands.contains(cmdId))
            {
                if (!allowCmdLoad(list[id], modName, exempt))
                {
                    termCommandObj(cmdId, commands[cmdId], true);
                }
            }
            else
            {
                if (rdSharedObjs->userName->isEmpty())
                {
                    if (pub.contains(list[id], Qt::CaseInsensitive))
                    {
                        loadCmd(loader, cmdId, modName, list[id], unique);
                    }
                }
                else if (allowCmdLoad(list[id], modName, exempt))
                {
                    loadCmd(loader, cmdId, modName, list[id], unique);
                }
            }
        }
        else
        {
            qDebug() << "CmdExecutor::getCmdNames() err: command object name '" << unique << "' is not valid.";
        }
    }
}

void CmdExecutor::buildCommands()
{
    for (auto&& loaderName : cmdLoaders.keys())
    {
        loadCmds(cmdLoaders[loaderName], getModIdOffs(loaderName), loaderName);
    }
}

bool CmdExecutor::externBlockedTypeId(uchar typeId)
{
    // the internal host objects will handle sending the following TypeIDs to the clients
    // and peers. any attempt to do so via ExternCommand object will be blocked since these
    // data types can cause some behaviour issues if sent at an unexpected time.

    return (typeId == PRIV_IPC)  || (typeId == PUB_IPC) || (typeId == PING_PEERS) ||
           (typeId == PEER_STAT) || (typeId == MY_INFO) || (typeId == PEER_INFO)  ||
           (typeId == HOST_CERT) || (typeId == IDLE)    || (typeId == NEW_CMD);
}

bool CmdExecutor::p2pBlockedTypeId(uchar typeId)
{
    // this is used to block P2P specific typeIDs. only toPeers() or internDataToIPC()
    // should be allowed to send these frame types, all others use this to block them.

    return (typeId == P2P_REQUEST) || (typeId == P2P_OPEN) || (typeId == P2P_CLOSE);
}

void CmdExecutor::externDataToIPC(quint16 cmdId, const QByteArray &data, uchar typeId)
{
    if (!externBlockedTypeId(typeId) && !p2pBlockedTypeId(typeId))
    {
        emit dataToSession(cmdId, data, typeId);
    }
}

void CmdExecutor::internDataToIPC(quint16 cmdId, const QByteArray &data, uchar typeId)
{
    if ((typeId != IDLE) && (typeId != NEW_CMD))
    {
        emit dataToSession(cmdId, data, typeId);
    }
}

void CmdExecutor::castToPeers(const QByteArray &data, uchar typeId)
{
    if (!externBlockedTypeId(typeId) && !p2pBlockedTypeId(typeId))
    {
        QByteArray castHeader = *rdSharedObjs->wrAbleChIds + wrInt(typeId, 8);

        emit dataToSession(ASYNC_CAST, castHeader + data, PUB_IPC);
    }
}

void CmdExecutor::toPeer(const QByteArray &dst, const QByteArray &data, uchar typeId)
{
    if (!externBlockedTypeId(typeId) && (dst.size() == 28))
    {
        QByteArray p2pHeader = dst + *rdSharedObjs->sessionId + wrInt(typeId, 8);

        if (typeId == P2P_REQUEST)
        {
            if (!rdSharedObjs->p2pPending->contains(dst))
            {
                rwSharedObjs->p2pPending->append(dst);

                emit dataToSession(ASYNC_P2P, p2pHeader + toPEER_INFO(rdSharedObjs), PUB_IPC);
            }
        }
        else
        {
            if ((typeId == P2P_CLOSE) || (typeId == P2P_OPEN))
            {
                if (rdSharedObjs->p2pPending->contains(dst) || rdSharedObjs->p2pAccepted->contains(dst))
                {
                    if (typeId == P2P_CLOSE)
                    {
                        rwSharedObjs->p2pPending->removeAll(dst);
                        rwSharedObjs->p2pAccepted->removeAll(dst);

                        emit dataToSession(ASYNC_P2P, p2pHeader + dst, PUB_IPC);
                    }
                    else if (!rdSharedObjs->p2pAccepted->contains(dst))
                    {
                        rwSharedObjs->p2pPending->removeAll(dst);
                        rwSharedObjs->p2pAccepted->append(dst);

                        emit dataToSession(ASYNC_P2P, p2pHeader + dst, PUB_IPC);
                    }
                }
            }
            else
            {
                emit dataToSession(ASYNC_P2P, p2pHeader + data, PUB_IPC);
            }
        }
    }
}

void CmdExecutor::backendFromCmd(quint16 cmdId, const QByteArray &data, uchar typeId)
{
    if ((typeId == PUB_IPC) || (typeId == PRIV_IPC) || (typeId == PUB_IPC_WITH_FEEDBACK))
    {
        emit dataToSession(cmdId, data, typeId);
    }
}

void CmdExecutor::openOrCloseChByName(quint16 cmdId, const QString &ch, const QString &sub, bool open)
{
    if (!validChName(ch))
    {
        emit dataToSession(cmdId, toTEXT("err: '" + ch + "' is not a valid channel name.\n"), ERR);
    }
    else if (!validChName(sub))
    {
        emit dataToSession(cmdId, toTEXT("err: '" + sub + "' is not a valid sub channel name.\n"), ERR);
    }
    else if (!channelSubExists(ch, sub))
    {
        emit dataToSession(cmdId, toTEXT("err: Sub-channel: '" + sub + "' does not exists.\n"), ERR);
    }
    else
    {
        Query db(this);

        db.setType(Query::PULL, TABLE_SUB_CHANNELS);
        db.addColumn(COLUMN_CHANNEL_ID);
        db.addColumn(COLUMN_SUB_CH_ID);
        db.addCondition(COLUMN_SUB_CH_NAME, sub);
        db.addCondition(COLUMN_CHANNEL_NAME, ch);
        db.exec();

        if (open)
        {
            openChById(cmdId, db.getData(COLUMN_CHANNEL_ID).toULongLong(), static_cast<uchar>(db.getData(COLUMN_SUB_CH_ID).toUInt()));
        }
        else
        {
            closeChById(cmdId, db.getData(COLUMN_CHANNEL_ID).toULongLong(), static_cast<uchar>(db.getData(COLUMN_SUB_CH_ID).toUInt()));
        }
    }
}

void CmdExecutor::openChByName(quint16 cmdId, const QString &ch, const QString &sub)
{
    openOrCloseChByName(cmdId, ch, sub, true);
}

void CmdExecutor::closeChByName(quint16 cmdId, const QString &ch, const QString &sub)
{
    openOrCloseChByName(cmdId, ch, sub, false);
}

void CmdExecutor::openChById(quint16 cmdId, quint64 chId, uchar subId)
{
    QByteArray id = wrInt(chId, 64) + wrInt(subId, 8);

    if (chId == 0)
    {
        emit dataToSession(cmdId, toTEXT("err: '0' is not a valid channel id. it must an unsigned integer between 1-18446744073709551615.\n"), ERR);
    }
    else if (countChs(*rdSharedObjs->chIds) == 6)
    {
        emit dataToSession(cmdId, toTEXT("err: The maximum amount of open sub-channels reached (6).\n"), ERR);
    }
    else if (containsChId(id, *rdSharedObjs->chIds))
    {
        emit dataToSession(cmdId, toTEXT("err: The requested sub-channel is already open.\n"), ERR);
    }
    else if (!channelSubExists(chId, subId))
    {
        emit dataToSession(cmdId, toTEXT("err: The requested sub-channel does not exists.\n"), ERR);
    }
    else if (channelAccessLevel(rdSharedObjs, chId) > lowestAcessLevel(chId, subId))
    {
        emit dataToSession(cmdId, toTEXT("err: Access denied.\n"), ERR);
    }
    else
    {
        wrOpenCh(rwSharedObjs, id);
    }
}

void CmdExecutor::closeChById(quint16 cmdId, quint64 chId, uchar subId)
{
    QByteArray id = wrInt(chId, 64) + wrInt(subId, 8);

    if (chId == 0)
    {
        emit dataToSession(cmdId, toTEXT("err: '0' is not a valid channel id. it must an integer between 1-18446744073709551615.\n"), ERR);
    }
    else if (!containsChId(id, *rdSharedObjs->chIds))
    {
        emit dataToSession(cmdId, toTEXT("err: The requested sub-channel is not open.\n"), ERR);
    }
    else
    {
        QByteArray peerStat;

        wrCloseCh(rwSharedObjs, id, peerStat);

        if (!peerStat.isEmpty())
        {
            emit dataToSession(ASYNC_LIMITED_CAST, peerStat, PUB_IPC);
        }
    }
}
