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
#include <QLibrary>

#include <openssl/ssl.h>
#include <openssl/x509.h>

#include "db.h"
#include "shell.h"
#include "mem_share.h"

#define FRAME_HEADER_SIZE 8
#define MAX_FRAME_BITS    24
#define LOCAL_BUFFSIZE    16777215
#define CLIENT_HEADER_LEN 292
#define SERVER_HEADER_TAG "MRCI"
#define HOST_CONTROL_PIPE "MRCI_HOST_CONTROL"

enum AsyncCommands : quint16
{
    ASYNC_RDY               = 1,   // client   | retricted
    ASYNC_SYS_MSG           = 2,   // client   | retricted
    ASYNC_EXIT              = 3,   // internal | private
    ASYNC_CAST              = 4,   // client   | public
    ASYNC_MAXSES            = 5,   // internal | private
    ASYNC_LOGOUT            = 6,   // internal | private
    ASYNC_USER_DELETED      = 7,   // client   | public
    ASYNC_DISP_RENAMED      = 8,   // internal | public
    ASYNC_USER_RANK_CHANGED = 9,   // internal | public
    ASYNC_CMD_RANKS_CHANGED = 10,  // internal | public
    ASYNC_RESTART           = 11,  // internal | private
    ASYNC_ENABLE_MOD        = 12,  // internal | public
    ASYNC_DISABLE_MOD       = 13,  // internal | public
    ASYNC_END_SESSION       = 14,  // internal | private
    ASYNC_USER_LOGIN        = 15,  // internal | private
    ASYNC_TO_PEER           = 16,  // client   | retricted
    ASYNC_LIMITED_CAST      = 17,  // client   | public
    ASYNC_RW_MY_INFO        = 18,  // internal | public
    ASYNC_P2P               = 19,  // client   | public
    ASYNC_CLOSE_P2P         = 20,  // internal | public
    ASYNC_NEW_CH_MEMBER     = 21,  // client   | public
    ASYNC_DEL_CH            = 22,  // client   | public
    ASYNC_RENAME_CH         = 23,  // client   | public
    ASYNC_CH_ACT_FLAG       = 24,  // internal | public
    ASYNC_NEW_SUB_CH        = 25,  // client   | public
    ASYNC_RM_SUB_CH         = 26,  // client   | public
    ASYNC_RENAME_SUB_CH     = 27,  // client   | public
    ASYNC_INVITED_TO_CH     = 28,  // client   | public
    ASYNC_RM_CH_MEMBER      = 29,  // client   | public
    ASYNC_INVITE_ACCEPTED   = 30,  // client   | public
    ASYNC_MEM_LEVEL_CHANGED = 31,  // client   | public
    ASYNC_SUB_CH_LEVEL_CHG  = 32,  // client   | public
    ASYNC_ADD_RDONLY        = 33,  // client   | public
    ASYNC_RM_RDONLY         = 34,  // client   | public
    ASYNC_ADD_CMD           = 35,  // client   | retricted
    ASYNC_RM_CMD            = 36,  // client   | retricted
    ASYNC_USER_RENAMED      = 37,  // internal | public
    ASYNC_PING_PEERS        = 38,  // internal | private
    ASYNC_OPEN_SUBCH        = 39,  // internal | private
    ASYNC_CLOSE_SUBCH       = 40,  // internal | private
    ASYNC_KEEP_ALIVE        = 42,  // internal | private
    ASYNC_SET_DIR           = 43,  // internal | private
    ASYNC_DEBUG_TEXT        = 44,  // internal | private
    ASYNC_HOOK_INPUT        = 45,  // internal | private
    ASYNC_UNHOOK            = 46   // internal | private
};

enum Flags : quint32
{
    FRAME_RDY                  = 1,
    SESSION_RDY                = 1 << 1,
    ACTIVE_PAYLOAD             = 1 << 2,
    END_SESSION_EMPTY_PROC     = 1 << 3,
    END_SESSION_ON_PAYLOAD_DEL = 1 << 4,
    RES_ON_EMPTY               = 1 << 5,
    CLOSE_ON_EMPTY             = 1 << 6,
    ACCEPTING                  = 1 << 7,
    LOADING_PUB_CMDS           = 1 << 8,
    LOADING_EXEMPT_CMDS        = 1 << 9,
    LOADING_USER_CMDS          = 1 << 10,
    SESSION_PARAMS_SET         = 1 << 11,
    LOGGED_IN                  = 1 << 12,
    MORE_INPUT                 = 1 << 13,
    LOOPING                    = 1 << 14,
    SINGLE_STEP_MODE           = 1 << 15,
    YIELD_STATE                = 1 << 16
};

enum FileInfoFlags : quint8
{
    IS_FILE   = 1,
    IS_DIR    = 1 << 1,
    IS_SYMLNK = 1 << 2,
    CAN_READ  = 1 << 3,
    CAN_WRITE = 1 << 4,
    CAN_EXE   = 1 << 5,
    EXISTS    = 1 << 6
};

enum TypeID : quint8
{
    GEN_FILE       = 1,
    TEXT           = 2,
    ERR            = 3,
    PRIV_TEXT      = 4,
    IDLE           = 5,
    HOST_CERT      = 6,
    FILE_INFO      = 7,
    PEER_INFO      = 8,
    MY_INFO        = 9,
    PEER_STAT      = 10,
    P2P_REQUEST    = 11,
    P2P_CLOSE      = 12,
    P2P_OPEN       = 13,
    BYTES          = 14,
    SESSION_ID     = 15,
    NEW_CMD        = 16,
    CMD_ID         = 17,
    BIG_TEXT       = 18,
    TERM_CMD       = 19,
    HOST_VER       = 20,
    PING_PEERS     = 21,
    CH_MEMBER_INFO = 22,
    CH_ID          = 23,
    KILL_CMD       = 24,
    YIELD_CMD      = 25,
    RESUME_CMD     = 26,
    PROMPT_TEXT    = 27,
    PROG           = 28,
    PROG_LAST      = 29,
    ASYNC_PAYLOAD  = 30
};

enum RetCode : quint16
{
    NO_ERRORS       = 1, // task execution completed without any issues.
    ABORTED         = 2, // the task aborted via user or host intervention.
    INVALID_PARAMS  = 3, // invalid/missing parameters prevented the task from executing.
    CRASH           = 4, // the command process has crashed.
    FAILED_TO_START = 5, // the command process could not start.
    EXECUTION_FAIL  = 6, // command specific error prevented execution of the task.
    CUSTOM          = 7  // indicates a custom return code.
};

enum GenFileType : quint8
{
    GEN_UPLOAD   = 2,
    GEN_DOWNLOAD = 3
};

enum ChannelMemberLevel : quint8
{
    OWNER   = 1,
    ADMIN   = 2,
    OFFICER = 3,
    REGULAR = 4,
    PUBLIC  = 5
};

class Session;

QByteArray  toFixedTEXT(const QString &txt, int len);
QByteArray  nullTermTEXT(const QString &txt);
QByteArray  rdFileContents(const QString &path, QTextStream &msg);
quint32     toCmdId32(quint16 cmdId, quint16 branchId);
quint16     toCmdId16(quint32 id);
void        printDatabaseInfo(QTextStream &txt);
void        serializeThread(QThread *thr);
void        mkPath(const QString &path);
void        listDir(QList<QPair<QString,QString> > &list, const QString &srcPath, const QString &dstPath);
void        containsActiveCh(const char *subChs, char *actBlock);
bool        acceptablePw(const QString &pw, const QByteArray &uId, QString *errMsg);
bool        acceptablePw(const QString &pw, const QString &uName, const QString &dispName, const QString &email, QString *errMsg);
bool        containsNewLine(const QString &str);
bool        validModPath(const QString &modPath);
bool        validUserName(const QString &uName);
bool        validEmailAddr(const QString &email);
bool        validPassword(const QString &pw);
bool        validCommandName(const QString &name);
bool        validDispName(const QString &name);
bool        validChName(const QString &name);
bool        validLevel(const QString &num, bool includePub);
bool        validSubId(const QString &num);
bool        modExists(const QString &modPath);
bool        userExists(const QString &uName, QByteArray *uId = nullptr, QString *email = nullptr);
bool        emailExists(const QString &email, QByteArray *uId = nullptr);
bool        inviteExists(const QByteArray &uId, quint64 chId);
bool        channelExists(const QString &chName, quint64 *chId = nullptr);
bool        channelSubExists(quint64 chId, const QString &sub, quint8 *subId = nullptr);
bool        recoverPWExists(const QByteArray &uId);
bool        rdOnlyFlagExists(quint64 chId, quint8 subId, quint32 level);
bool        isBool(const QString &str);
bool        isInt(const QString &str);
bool        isLocked(const QByteArray &uId);
bool        matchedFsObjTypes(const QString &pathA, const QString &pathB);
bool        matchedVolume(const QString &pathA, const QString &pathB);
bool        noCaseMatch(const QString &strA, const QString &strB);
bool        argExists(const QString &key, const QStringList &args);
bool        matchAnyCh(const char *chsA, const char *chsB);
bool        fullMatchChs(const char *openChs, const char *comp);
bool        globalActiveFlag();
bool        genSubId(quint64 chId, quint8 *newId);
bool        isChOwner(const QByteArray &uId);
int         channelAccessLevel(const QByteArray &uId, quint64 chId);
int         channelAccessLevel(const QByteArray &uId, const char *override, quint64 chId);
int         maxSubChannels();
QString     getUserNameForEmail(const QString &email);
QString     getEmailForUser(const QByteArray &uId);
QString     getDispName(const QByteArray &uId);
QString     getUserName(const QByteArray &uId);
QString     boolStr(bool state);
QString     getParam(const QString &key, const QStringList &args);
QString     escapeChars(const QString &str, const QChar &escapeChr, const QChar &chr);
QString     genSerialNumber();
QString     defaultPw();
QString     sslCertChain();
QString     sslPrivKey();
QStringList parseArgs(const QByteArray &data, int maxArgs, int *pos = nullptr);

//---------------------------

class SessionCarrier : public QObject
{
    Q_OBJECT

public:

    Session *sessionObj;

    explicit SessionCarrier(Session *session);
};

//----------------------------

class IdleTimer : public QTimer
{
    Q_OBJECT

private slots:

    void detectWrite(qint64);

public:

    explicit IdleTimer(QObject *parent = nullptr);

    void attach(QIODevice *dev, int msec);
};

//---------------------------

class ShellIPC : public QLocalSocket
{
    Q_OBJECT

private:

    QStringList arguments;
    bool        holdErrs;

private slots:

    void dataIn();
    void hostConnected();

public:

    bool connectToHost();

    explicit ShellIPC(const QStringList &args, bool supressErr = false, QObject *parent = nullptr);

signals:

    void closeInstance();
};

//--------------------------

class Serial
{

public:

    static quint64 serialIndex;
};

#endif // COMMON_H
