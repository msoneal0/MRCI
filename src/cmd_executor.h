#ifndef CMD_EXECUTOR_H
#define CMD_EXECUTOR_H

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

class CmdExecutor : public QObject
{
    Q_OBJECT

private:

    SharedObjs                     *rdSharedObjs;
    RWSharedObjs                   *rwSharedObjs;
    InternalCommandLoader          *internalCmds;
    QSharedMemory                  *exeDebugInfo;
    QHash<QString, CommandLoader*>  cmdLoaders;
    QHash<QString, QPluginLoader*>  plugins;
    QHash<QString, QList<quint16> > cmdIdsByModName;
    QList<quint16>                  moreInputCmds;
    QList<quint16>                  activeLoopCmds;
    QList<quint16>                  pausedCmds;
    QHash<quint16, QString>         cmdNames;
    QHash<quint16, ExternCommand*>  commands;
    int                             loopIndex;

    void    nextLoopCmd();
    void    preExe(ExternCommand *cmdObj, quint16 cmdId);
    void    preExe(const QList<ExternCommand*> &cmdObjs, quint16 cmdId);
    void    procInternRequest(ExternCommand *cmd, quint16 cmdId, const QString &cmdName);
    void    connectCommon(ExternCommand *cmd, CommandOutput *cmdOutput, quint16 cmdId, const QString &cmdName);
    void    connectInternCmd(InternCommand *cmd, quint16 cmdId, const QString &cmdName);
    void    connectExternCmd(ExternCommand *cmd, quint16 cmdId, const QString &cmdName);
    void    openOrCloseChByName(quint16 cmdId, const QString &ch, const QString &sub, bool open);
    void    loadCmd(CommandLoader *loader, quint16 cmdId, const QString &modName, const QString &cmdName, const QString &uniqueCmdName);
    void    loadCmds(CommandLoader *loader, quint16 idOffs, const QString &modName);
    void    loadInternCmd(CommandLoader *loader, const QString &cmdName, const QString &uniqueName, quint16 id);
    void    loadExternCmd(CommandLoader *loader, const QString &modName, const QString &cmdName, const QString &uniqueName, quint16 id);
    void    addCommandToList(quint16 cmdId, const QString &cmdName, const QString &modName, ExternCommand *cmdObj);
    bool    externBlockedTypeId(uchar typeId);
    bool    p2pBlockedTypeId(uchar typeId);
    bool    allowCmdLoad(const QString &cmdName, const QString &modName, const QStringList &exemptList);
    QString makeCmdUnique(const QString &name);
    QString getModFile(const QString &modName);
    quint16 getModIdOffs(const QString &name);

private slots:

    void termAllCommands();
    void enableMoreInput(quint16 cmdId, bool state);
    void enableLoop(quint16 cmdId, bool state);
    void openChById(quint16 cmdId, quint64 chId, uchar subId);
    void openChByName(quint16 cmdId, const QString &ch, const QString &sub);
    void closeChById(quint16 cmdId, quint64 chId, uchar subId);
    void closeChByName(quint16 cmdId, const QString &ch, const QString &sub);
    void termCommandId(quint16 cmdId);
    void termCommandObj(quint16 cmdId, ExternCommand *cmd, bool del = false);
    void termCommandsInList(const QList<quint16> &cmds, bool del = false);
    void commandFinished(quint16 cmdId);
    void externDataToIPC(quint16 cmdId, const QByteArray &data, uchar typeId);
    void internDataToIPC(quint16 cmdId, const QByteArray &data, uchar typeId);
    void castToPeers(const QByteArray &data, uchar typeId);
    void toPeer(const QByteArray &dst, const QByteArray &data, uchar typeId);

public slots:

    void close();
    void buildCommands();
    void buildCmdLoaders();
    void wrCrashDebugInfo(const QString &msg);
    void unloadModFile(const QString &modName);
    void loadModFile(const QString &modName);
    void exeCmd(quint16 cmdId, const QByteArray &data, uchar typeId);
    void backendFromCmd(quint16 cmdId, const QByteArray &data, uchar typeId);

signals:

    void endSession();
    void logout();
    void authOk();
    void okToDelete();
    void dataToSession(quint16 cmdId, const QByteArray &data, uchar typeId);
    void loop(quint16 cmdId, const QByteArray &data, uchar typeId);

public:

    explicit CmdExecutor(RWSharedObjs *rwShare, SharedObjs *rdOnlyShare, QSharedMemory *debugInfo, QObject *parent = nullptr);
};

#endif // CMD_EXECUTOR_H
