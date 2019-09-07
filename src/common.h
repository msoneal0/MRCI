#ifndef COMMON_H
#define COMMON_H

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

#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <QList>
#include <QObject>
#include <QtEndian>
#include <QSharedMemory>
#include <QSharedPointer>
#include <QTextStream>
#include <QRandomGenerator>
#include <QProcess>
#include <QHash>
#include <QRegExp>
#include <QStringList>
#include <QDateTime>
#include <QHostAddress>
#include <QCoreApplication>
#include <QTextCodec>
#include <QFileInfo>
#include <QDir>
#include <QSysInfo>
#include <QFileInfoList>
#include <QTemporaryFile>
#include <QPluginLoader>
#include <QChar>
#include <QtMath>
#include <QStorageInfo>
#include <QPair>
#include <QLocalSocket>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QTimer>
#include <QThread>
#include <QSslSocket>
#include <QPluginLoader>
#include <QAbstractSocket>
#include <QSslCertificate>
#include <QSslKey>
#include <QLocalServer>
#include <QSslError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QReadWriteLock>
#include <QTextStream>
#include <QSqlDatabase>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageLogContext>
#include <QtGlobal>

#include "db.h"
#include "shell.h"
#include "commands/command.h"

#define FRAME_HEADER_SIZE   6
#define MAX_FRAME_BITS      24
#define IMPORT_REV          1
#define LOCAL_BUFFSIZE      16777215
#define CLIENT_INIT_TIME    5000
#define IPC_PREP_TIME       1000
#define IPC_CONNECT_DELAY   500
#define CLIENT_HEADER_LEN   410
#define SERVER_HEADER_LEN   35
#define EXE_CRASH_LIMIT     5
#define EXE_DEBUG_INFO_SIZE 512
#define SERVER_HEADER_TAG   "MRCI"

#define ASYNC_RDY                1
#define ASYNC_SYS_MSG            2
#define ASYNC_EXE_CRASH          3
#define ASYNC_EXIT               4   // internal only
#define ASYNC_CAST               5   // internal only
#define ASYNC_MAXSES             6   // internal only
#define ASYNC_LOGOUT             7   // internal only
#define ASYNC_USER_DELETED       8   // internal only
#define ASYNC_GROUP_RENAMED      9   // internal only
#define ASYNC_DISP_RENAMED       10  // internal only
#define ASYNC_GRP_TRANS          11  // internal only
#define ASYNC_USER_GROUP_CHANGED 12  // internal only
#define ASYNC_CMD_RANKS_CHANGED  13  // internal only
#define ASYNC_RESTART            14  // internal only
#define ASYNC_ENABLE_MOD         15  // internal only
#define ASYNC_DISABLE_MOD        16  // internal only
#define ASYNC_GROUP_UPDATED      17  // internal only
#define ASYNC_END_SESSION        18  // internal only
#define ASYNC_USER_LOGIN         19  // internal only
#define ASYNC_RESTORE_AUTH       20  // internal only
#define ASYNC_TO_PEER            21
#define ASYNC_LIMITED_CAST       22
#define ASYNC_RW_MY_INFO         23  // internal only
#define ASYNC_P2P                24
#define ASYNC_CLOSE_P2P          25  // internal only
#define ASYNC_NEW_CH_MEMBER      26
#define ASYNC_DEL_CH             27
#define ASYNC_RENAME_CH          28
#define ASYNC_CH_ACT_FLAG        29
#define ASYNC_NEW_SUB_CH         30
#define ASYNC_RM_SUB_CH          31
#define ASYNC_RENAME_SUB_CH      32
#define ASYNC_INVITED_TO_CH      33
#define ASYNC_RM_CH_MEMBER       34
#define ASYNC_INVITE_ACCEPTED    35
#define ASYNC_MEM_LEVEL_CHANGED  36
#define ASYNC_SUB_CH_LEVEL_CHG   37
#define ASYNC_ADD_RDONLY         38
#define ASYNC_RM_RDONLY          39
#define ASYNC_ADD_CMD            40
#define ASYNC_RM_CMD             41
#define ASYNC_USER_RENAMED       42
#define ASYNC_PUBLIC_AUTH        43  // internal only

enum SalveExitCodes
{
    GEN_ERR              = 1,
    FAILED_TO_OPEN_PIPE  = 2,
    PIPE_CONNECT_TIMEOUT = 3
};

enum PrivateTypeID
{
    PRIV_IPC              = 1,
    PUB_IPC               = 2,
    PUB_IPC_WITH_FEEDBACK = 3,
    PING_PEERS            = 4
};

enum Flags : uint
{
    IPC_LINK_OK                = 1,
    IPC_FRAME_RDY              = 1 << 1,
    TCP_FRAME_RDY              = 1 << 2,
    SSL_HOLD                   = 1 << 3,
    VER_OK                     = 1 << 4,
    EXPECTED_TERM              = 1 << 5,
    ACTIVE_PAYLOAD             = 1 << 6,
    END_SESSION_ON_PAYLOAD_DEL = 1 << 7,
    RES_ON_EMPTY               = 1 << 8,
    CLOSE_ON_EMPTY             = 1 << 9,
    ACCEPTING                  = 1 << 10
};

enum FileInfoFlags
{
    IS_FILE   = 1,
    IS_DIR    = 1 << 1,
    IS_SYMLNK = 1 << 2,
    CAN_READ  = 1 << 3,
    CAN_WRITE = 1 << 4,
    CAN_EXE   = 1 << 5,
    EXISTS    = 1 << 6
};

typedef ExternCommand* (*BuildInternCmd)(QObject*);

class RWSharedObjs;
class Session;

QByteArray  wrFrame(quint16 cmdId, const QByteArray &data, uchar dType);
QByteArray  wrInt(quint64 num, int numOfBits);
QByteArray  wrInt(qint64 num, int numOfBits);
QByteArray  wrInt(int num, int numOfBits);
QByteArray  wrInt(uint num, int numOfBits);
QByteArray  toFILE_INFO(const QString &path);
QByteArray  toFILE_INFO(const QFileInfo &info);
QByteArray  toTEXT(const QString &txt);
QByteArray  fixedToTEXT(const QString &txt, int len);
QByteArray  toPEER_INFO(const SharedObjs *sharedObjs);
QByteArray  toMY_INFO(const SharedObjs *sharedObjs);
QByteArray  toPEER_STAT(const QByteArray &sesId, const QByteArray &chIds, bool isDisconnecting);
QByteArray  toNEW_CMD(quint16 cmdId, const QString &cmdName, ExternCommand *cmdObj);
quint64     rdInt(const QByteArray &bytes);
quint64     getChId(const QString &chName);
uchar       getSubId(const QString &chName, const QString &subName);
uint        rdSessionLoad();
uint        getRankForGroup(const QString &grName);
void        wrSessionLoad(uint value);
void        serializeThread(QThread *thr);
void        uniqueAdd(quint16 id, QList<quint16> &list);
void        mkPathForFile(const QString &path);
void        msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
void        mkPath(const QString &path);
void        mkFile(const QString &path);
void        moveCharLeft(int pos, QString &str);
void        moveCharRight(int pos, QString &str);
void        listDir(QList<QPair<QString,QString> > &list, const QString &srcPath, const QString &dstPath);
void        wrOpenCh(RWSharedObjs *sharedObjs, const QByteArray &id);
void        wrCloseCh(RWSharedObjs *sharedObjs, const QByteArray &id, QByteArray &peerStat);
void        wrCloseCh(RWSharedObjs *sharedObjs, quint64 chId, QByteArray &peerStat);
void        wrCloseCh(RWSharedObjs *sharedObjs, const QByteArray &id);
void        wrCloseCh(RWSharedObjs *sharedObjs, quint64 chId);
bool        containsNewLine(const QString &str);
bool        validUserName(const QString &uName);
bool        validEmailAddr(const QString &email);
bool        validPassword(const QString &pw);
bool        validGroupName(const QString &grName);
bool        validCommandName(const QString &name);
bool        validCommonName(const QString &name);
bool        validDispName(const QString &name);
bool        validChName(const QString &name);
bool        validLevel(const QString &num, bool includePub);
bool        validSubId(const QString &num);
bool        modExists(const QString &modName);
bool        userExists(const QString &uName);
bool        emailExists(const QString &email);
bool        groupExists(const QString &grName);
bool        inviteExists(const QString &uName, const QString &chName);
bool        memberExists(const QString &uName, const QString &chName);
bool        channelExists(quint64 chId);
bool        channelExists(const QString &chName);
bool        channelSubExists(quint64 chId, uchar subId);
bool        channelSubExists(const QString &ch, const QString &sub);
bool        recoverPWExists(const QString &uName);
bool        rdOnlyFlagExists(const QString &chName, uchar subId, int level);
bool        checkRank(const QString &myGroup, const QString &targetGroup, bool equalAcceptable = false);
bool        isBool(const QString &str);
bool        isInt(const QString &str);
bool        isLocked(const QString &uName);
bool        commandHasRank(const QString &cmdName);
bool        matchedFsObjTypes(const QString &pathA, const QString &pathB);
bool        matchedVolume(const QString &pathA, const QString &pathB);
bool        noCaseMatch(const QString &strA, const QString &strB);
bool        argExists(const QString &key, const QStringList &args);
bool        matchChs(const QByteArray &chsA, const QByteArray &chsB);
bool        containsChId(const QByteArray &chId, const QByteArray &chIds);
bool        containsActiveCh(const QByteArray &chIds);
bool        globalActiveFlag();
bool        genSubId(const QString &chName, int *newId);
bool        isChOwner(const QString &uName);
bool        allowMemberDel(const SharedObjs *sharedObjs, const QString &targetUName, const QString &chName);
bool        allowLevelChange(const SharedObjs *sharedObjs, int newLevel, const QString &chName);
bool        maxedInstalledMods();
int         channelAccessLevel(const QString &uName, quint64 chId);
int         channelAccessLevel(const QString &uName, const QString &chName);
int         channelAccessLevel(const SharedObjs *sharedObjs, const QString &chName);
int         channelAccessLevel(const SharedObjs *sharedObjs, quint64 chId);
int         lowestAcessLevel(quint64 chId, uchar subId);
int         chPos(const QByteArray &id, const QByteArray &chIds);
int         blankChPos(const QByteArray &chIds);
int         countChs(const QByteArray &chIds);
int         inRange(int pos, int min, int max);
int         maxSubChannels();
QString     fromTEXT(const QByteArray &txt);
QString     getUserGroup(const QString &uName);
QString     getUserNameForEmail(const QString &email);
QString     getEmailForUser(const QString &uName);
QString     boolStr(bool state);
QString     getParam(const QString &key, const QStringList &args);
QString     genPw();
QString     escapeChars(const QString &str, const QChar &escapeChr, const QChar &chr);
QString     genSerialNumber();
QString     modDataPath();
QString     pipesPath();
QString     sessionCountShareKey();
QStringList parseArgs(const QByteArray &data, int maxArgs, int *pos = nullptr);
QList<int>  genSequence(int min, int max, int len);
QChar       genLetter();
QChar       genNum();
QChar       genSpecialChar();

class RWSharedObjs : public QObject
{
    Q_OBJECT

public:

    QHash<quint16, ExternCommand*> *commands;
    QHash<quint16, QString>        *cmdNames;
    QList<QString>                 *chList;
    QList<QByteArray>              *p2pAccepted;
    QList<QByteArray>              *p2pPending;
    QList<quint16>                 *moreInputCmds;
    QList<quint16>                 *activeLoopCmds;
    QList<quint16>                 *pausedCmds;
    QString                        *sessionAddr;
    QString                        *userName;
    QString                        *groupName;
    QString                        *displayName;
    QString                        *appName;
    ushort                         *clientMajor;
    ushort                         *clientMinor;
    ushort                         *clientPatch;
    QByteArray                     *chIds;
    QByteArray                     *wrAbleChIds;
    QByteArray                     *sessionId;
    QByteArray                     *userId;
    bool                           *activeUpdate;
    bool                           *chOwnerOverride;
    uint                           *hostRank;

    explicit RWSharedObjs(QObject *parent = nullptr);
};

//---------------------------

class SessionCarrier : public QObject
{
    Q_OBJECT

public:

    Session *sessionObj;

    explicit SessionCarrier(Session *session);
};

//--------------------------

class InternCommand : public ExternCommand
{
    Q_OBJECT

protected:

    RWSharedObjs *rwSharedObjs;

    bool    loopEnabled();
    bool    moreInputEnabled();
    QString parseMd(int offset);

public:

    void    setWritableDataShare(RWSharedObjs *sharedObjs);
    QString libText();
    QString shortText();
    QString ioText();
    QString longText();

    explicit InternCommand(QObject *parent = nullptr);

signals:

    void authOk();
    void termAllCommands();
    void castPeerInfo();
    void reloadCommands();
    void loadMod(const QString &path);
    void unloadMod(const QString &path);
    void termCommandId(quint16 cmdId);
    void backendDataOut(quint16 cmdId, const QByteArray &data, uchar typeId);
};

//----------------------------

class CommandOutput : public QObject
{
    Q_OBJECT

private:

    quint16        cmdId;
    ExternCommand *cmdObj;

public:

    explicit CommandOutput(ExternCommand *parent);

    void setCmdId(quint16 id);

public slots:

    void dataFromCmdObj(const QByteArray &data, uchar typeId);
    void openChIdFromCmdObj(quint64 id, uchar subId);
    void closeChIdFromCmdObj(quint64 id, uchar subId);
    void openChNameFromCmdObj(const QString &ch, const QString &sub);
    void closeChNameFromCmdObj(const QString &ch, const QString &sub);
    void enableLoopFromCmdObj(bool state);
    void enableMoreInputFromCmdObj(bool state);
    void finished();

signals:

    void dataOut(quint16 cmdId, const QByteArray &data, uchar typeId);
    void openChById(quint16 cmdId, quint64 id, uchar subId);
    void closeChById(quint16 cmdId, quint64 id, uchar subId);
    void openChByName(quint16 cmdId, const QString &ch, const QString &sub);
    void closeChByName(quint16 cmdId, const QString &ch, const QString &sub);
    void cmdFinished(quint16 cmdId);
    void enableLoop(quint16 cmdId, bool state);
    void enableMoreInput(quint16 cmdId, bool state);
};

//----------------------------

class ShellIPC : public QLocalSocket
{
    Q_OBJECT

private:

    QStringList arguments;
    QString     pipeName;

private slots:

    void dataIn();
    void hostConnected();

public:

    bool connectToHost();

    explicit ShellIPC(const QStringList &args, QObject *parent = nullptr);

signals:

    void closeInstance();
};

#endif // COMMON_H
