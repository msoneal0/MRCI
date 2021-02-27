#ifndef CMD_PROC_H
#define CMD_PROC_H

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
#include "db.h"

class ModProcess : public QProcess
{
    Q_OBJECT

private:

    QHash<QString, quint32>      cmdRanks;
    QHash<QString, QStringList> *modCmdNames;
    QHash<quint16, QString>     *cmdUniqueNames;
    QHash<quint16, QString>     *cmdRealNames;
    QHash<quint16, QString>     *cmdAppById;
    QList<quint16>              *cmdIds;

    quint16 genCmdId();
    QString makeCmdUnique(const QString &name);
    bool    allowCmdLoad(const QString &cmdName);

protected:

    QString       pipeName;
    QString       fullPipe;
    QString       sesMemKey;
    QString       hostMemKey;
    quint8        ipcTypeId;
    quint32       ipcDataSize;
    quint32       hostRank;
    quint32       flags;
    QStringList   additionalArgs;
    IdleTimer    *idleTimer;
    QLocalServer *ipcServ;
    QLocalSocket *ipcSocket;

    virtual void onReady();
    virtual void onFailToStart();
    virtual void onDataFromProc(quint8 typeId, const QByteArray &data);

    void cleanupPipe();
    void logErrMsgs(quint32 id);
    void wrIpcFrame(quint8 typeId, const QByteArray &data);
    bool startProc(const QStringList &args);
    bool isCmdLoaded(const QString &name);
    bool openPipe();

protected slots:

    virtual void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    virtual void rdFromStdErr();
    virtual void rdFromStdOut();

    void rdFromIPC();
    void newIPCLink();
    void ipcDisconnected();
    void err(QProcess::ProcessError error);

public:

    explicit ModProcess(const QString &app, const QString &memSes, const QString &memHos, const QString &pipe, QObject *parent = nullptr);

    void addArgs(const QString &cmdLine);
    void setSessionParams(QHash<quint16, QString> *uniqueNames,
                          QHash<quint16, QString> *realNames,
                          QHash<quint16, QString> *appById,
                          QHash<QString, QStringList> *namesForMod,
                          QList<quint16> *ids,
                          quint32 rnk);

    bool loadPublicCmds();
    bool loadUserCmds();
    bool loadExemptCmds();

public slots:

    void killProc();

signals:

    void modProcFinished();
    void cmdUnloaded(quint16 cmdId);
    void dataToClient(quint32 cmdId, const QByteArray &data, quint8 typeId);
};

//----------------------------

class CmdProcess : public ModProcess
{
    Q_OBJECT

private:

    quint32        cmdId;
    QString        cmdName;
    bool           cmdIdle;
    quint32       *hook;
    QSharedMemory *sesMem;
    char          *sessionId;
    char          *openWritableSubChs;

    void onReady();
    void onFailToStart();
    void asyncDirector(quint16 id, const QByteArray &payload);
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onDataFromProc(quint8 typeId, const QByteArray &data);
    bool validAsync(quint16 async, const QByteArray &data, QTextStream &errMsg);

private slots:

    void rdFromStdOut();
    void rdFromStdErr();

public slots:

    void killCmd16(quint16 id16);
    void killCmd32(quint32 id32);

public:

    explicit CmdProcess(quint32 id, const QString &cmd, const QString &modApp, const QString &memSes, const QString &memHos, const QString &pipe, QObject *parent = nullptr);

    void dataFromSession(quint32 id, const QByteArray &data, quint8 dType);
    void setSessionParams(QSharedMemory *mem, char *sesId, char *wrableSubChs, quint32 *hookCmd);
    bool startCmdProc();

signals:

    void cmdProcFinished(quint32 id);
    void cmdProcReady(quint32 id);
    void pubIPC(quint16 cmdId, const QByteArray &data);
    void privIPC(quint16 cmdId, const QByteArray &data);
    void pubIPCWithFeedBack(quint16 cmdId, const QByteArray &data);
};

#endif // CMD_PROC_H
