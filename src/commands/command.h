#ifndef EXTERN_COMMAND_H
#define EXTERN_COMMAND_H

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

#define TXT_CODEC      "UTF-16LE"
#define TXT_CODEC_BITS 16
#define MOD_LOADER_IID "MCRI.host.module"

#include <QObject>
#include <QTextCodec>
#include <QCoreApplication>
#include <QHash>
#include <QPluginLoader>

enum TypeID
{
    GEN_FILE    = 30,
    TEXT        = 31,
    ERR         = 32,
    PRIV_TEXT   = 33,
    IDLE        = 34,
    HOST_CERT   = 35,
    FILE_INFO   = 36,
    PEER_INFO   = 37,
    MY_INFO     = 38,
    PEER_STAT   = 39,
    P2P_REQUEST = 40,
    P2P_CLOSE   = 41,
    P2P_OPEN    = 42,
    BYTES       = 43,
    SESSION_ID  = 44,
    NEW_CMD     = 45,
    CMD_ID      = 46,
    BIG_TEXT    = 47
};

enum ChannelMemberLevel
{
    OWNER   = 1,
    ADMIN   = 2,
    OFFICER = 3,
    REGULAR = 4,
    PUBLIC  = 5
};

class ExternCommand;

class SharedObjs : public QObject
{
    Q_OBJECT

public:

    const QHash<quint16, QString> *cmdNames;
    const QList<QString>          *chList;
    const QList<QByteArray>       *p2pAccepted;
    const QList<QByteArray>       *p2pPending;
    const QList<quint16>          *moreInputCmds;
    const QList<quint16>          *activeLoopCmds;
    const QList<quint16>          *pausedCmds;
    const QString                 *sessionAddr;
    const QString                 *userName;
    const QString                 *groupName;
    const QString                 *displayName;
    const QString                 *appName;
    const ushort                  *clientMajor;
    const ushort                  *clientMinor;
    const ushort                  *clientPatch;
    const QByteArray              *chIds;
    const QByteArray              *wrAbleChIds;
    const QByteArray              *sessionId;
    const QByteArray              *userId;
    const bool                    *activeUpdate;
    const bool                    *chOwnerOverride;
    const uint                    *hostRank;

    explicit SharedObjs(QObject *parent = nullptr);
};

class ExternCommand : public QObject
{
    Q_OBJECT

protected:

    void mainTxt(const QString &txt);
    void errTxt(const QString &txt);
    void privTxt(const QString &txt);
    void bigTxt(const QString &txt);

public:

    explicit ExternCommand(QObject *parent = nullptr) : QObject(parent) {}

    virtual ~ExternCommand() {}

    virtual void        procBin(const SharedObjs *, const QByteArray &, uchar) {}
    virtual void        aboutToDelete() {}
    virtual void        term() {}
    virtual bool        handlesGenfile() {return false;}
    virtual bool        errState();
    virtual QString     shortText() {return "";}
    virtual QString     ioText()    {return "";}
    virtual QString     longText()  {return "";}
    virtual QString     libText()   {return "";}
    virtual QStringList internRequest() {return QStringList();}

    QHash<QString, ExternCommand*> internCommands;
    quint16                        cmdId;
    bool                           errSent;
    bool                           inLoopMode;
    bool                           inMoreInputMode;

signals:

    void dataToClient(const QByteArray &data, uchar typeId = TEXT);
    void castToPeers(const QByteArray &data, uchar typeId = TEXT);
    void toPeer(const QByteArray &dst, const QByteArray &data, uchar typeId = TEXT);
    void closeChByName(const QString &ch, const QString &sub);
    void closeChById(quint64 id, uchar subId);
    void openChByName(const QString &ch, const QString &sub);
    void openChById(quint64 id, uchar subId);
    void enableLoop(bool state);
    void enableMoreInput(bool state);
    void closeSession();
    void cmdFinished();
    void logout();
};

class CommandLoader : public QObject
{
    Q_OBJECT

public:

    explicit CommandLoader(QObject *parent = nullptr) : QObject(parent) {}

    virtual ~CommandLoader() {}

    virtual QStringList    pubCmdList()     {return QStringList();}
    virtual QStringList    cmdList()        {return QStringList();}
    virtual QStringList    rankExemptList() {return QStringList();}
    virtual ExternCommand *cmdObj(const QString &) {return nullptr;}
};

class ModCommandLoader : public CommandLoader
{
    Q_OBJECT

public:

    explicit ModCommandLoader(QObject *parent = nullptr) : CommandLoader(parent) {}

    virtual ~ModCommandLoader() {}

    virtual void    modPath(const QString &) {}
    virtual void    aboutToDelete() {}
    virtual bool    hostRevOk(quint64) {return false;}
    virtual QString lastError()        {return "";}
    virtual quint64 rev()              {return 0;}
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(ModCommandLoader, MOD_LOADER_IID)
QT_END_NAMESPACE

#endif // EXTERN_COMMAND_H
