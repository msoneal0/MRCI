#ifndef DB_H
#define DB_H

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

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>
#include <QList>
#include <QSqlError>
#include <QDir>
#include <QCoreApplication>
#include <QFile>
#include <QTextCodec>
#include <QThread>
#include <QCryptographicHash>
#include <QTextStream>
#include <QStringList>
#include <QProcess>
#include <QDateTime>
#include <QRandomGenerator>

#include "shell.h"

#define APP_NAME   "MRCI"
#define APP_VER    "2.1.2"
#define APP_TARGET "mrci"

#ifdef Q_OS_WIN

#define DEFAULT_MAILBIN   "%COMSPEC%"
#define DEFAULT_MAIL_SEND "echo %message_body% | mutt -s %subject% %target_email%"
#define DEFAULT_DB_PATH   "%LOCALAPPDATA%\\%EXENAME%\\data.db"

#else

#define DEFAULT_MAILBIN   "/bin/sh"
#define DEFAULT_MAIL_SEND "-c \"echo %message_body% | mutt -s %subject% %target_email%\""
#define DEFAULT_DB_PATH   "$HOME/.$EXENAME/data.db"

#endif

#define ENV_DB_PATH             "MRCI_DB_PATH"
#define ENV_EXENAME             "EXENAME"
#define ROOT_USER               "root"
#define SUBJECT_SUB             "%subject%"
#define MSG_SUB                 "%message_body%"
#define TARGET_EMAIL_SUB        "%target_email%"
#define CONFIRM_CODE_SUB        "%confirmation_code%"
#define TEMP_PW_SUB             "%temp_pw%"
#define USERNAME_SUB            "%user_name%"
#define DATE_SUB                "%date%"
#define INTERN_MOD_NAME         ":internal_mod"
#define DEFAULT_CONFIRM_SUBJECT "Email Verification"
#define DEFAULT_TEMP_PW_SUBJECT "Password Reset"
#define DEFAULT_LISTEN_ADDRESS  "0.0.0.0"
#define DEFAULT_LISTEN_PORT     35516
#define DEFAULT_BAN_LIMIT       30
#define DEFAULT_LOCK_LIMIT      20
#define DEFAULT_MAXSESSIONS     100
#define DEFAULT_MAX_SUBS        50
#define DEFAULT_INIT_RANK       2
#define MAX_LS_ENTRIES          50
#define MAX_CMDS_PER_MOD        256

#define TXT_TempPwTemplate "\
A password reset was requested for account: %user_name%\n\
Your recovery password is as follows:\n\n\
%temp_pw%\n\n\
PLEASE IGNORE THIS EMAIL IF YOU MADE NO SUCH REQUEST.\n\n\
Date requested: %date%."

#define TXT_ConfirmCodeTemplate "\
Please confirm your email address for account: %user_name%\n\
Your confirmation code is as follows:\n\n\
%confirmation_code%\n\n\
Date requested: %date%."

#define TABLE_IPHIST        "ip_history"
#define TABLE_IPBANS        "ban_list"
#define TABLE_USERS         "users"
#define TABLE_SERV_SETTINGS "host_settings"
#define TABLE_CMD_RANKS     "command_ranks"
#define TABLE_AUTH_LOG      "auth_log"
#define TABLE_PW_RECOVERY   "pw_recovery"
#define TABLE_CERT_DATA     "ssl_certs"
#define TABLE_DMESG         "host_debug_messages"
#define TABLE_CHANNELS      "channels"
#define TABLE_CH_MEMBERS    "channel_members"
#define TABLE_SUB_CHANNELS  "sub_channels"
#define TABLE_RDONLY_CAST   "read_only_flags"
#define TABLE_MODULES       "modules"

#define COLUMN_IPADDR          "ip_address"
#define COLUMN_LOGENTRY        "log_entry"
#define COLUMN_CLIENT_VER      "client_version"
#define COLUMN_SESSION_ID      "session_id"
#define COLUMN_TIME            "time_stamp"
#define COLUMN_USERNAME        "user_name"
#define COLUMN_PORT            "port"
#define COLUMN_BAN_LIMIT       "auto_ban_threshold"
#define COLUMN_LOCK_LIMIT      "auto_lock_threshold"
#define COLUMN_MAXSESSIONS     "max_sessions"
#define COLUMN_HOST_RANK       "host_rank"
#define COLUMN_COMMAND         "command_name"
#define COLUMN_MOD_MAIN        "module_executable"
#define COLUMN_INITRANK        "initial_host_rank"
#define COLUMN_PUB_USERS       "allow_public_registrations"
#define COLUMN_SALT            "salt"
#define COLUMN_HASH            "hash"
#define COLUMN_EMAIL           "email_address"
#define COLUMN_NEED_PASS       "new_password_req"
#define COLUMN_NEED_NAME       "new_name_req"
#define COLUMN_EMAIL_VERIFIED  "email_verified"
#define COLUMN_LOCKED          "locked"
#define COLUMN_ZIPBIN          "archiver_executable"
#define COLUMN_ZIPEXTRACT      "extract_command"
#define COLUMN_ZIPCOMPRESS     "compress_command"
#define COLUMN_CERT            "ssl_cert"
#define COLUMN_PRIV_KEY        "private_key"
#define COLUMN_AUTH_ATTEMPT    "auth_attempt"
#define COLUMN_RECOVER_ATTEMPT "recover_attempt"
#define COLUMN_COUNT           "count_to_threshold"
#define COLUMN_ACCEPTED        "accepted"
#define COLUMN_MAILERBIN       "mailer_executable"
#define COLUMN_MAIL_SEND       "mailer_send_command"
#define COLUMN_TEMP_PW_MSG     "temp_pw_email_template"
#define COLUMN_CONFIRM_MSG     "verify_email_template"
#define COLUMN_CONFIRM_SUBJECT "verify_email_subject"
#define COLUMN_TEMP_PW_SUBJECT "temp_pw_email_subject"
#define COLUMN_ENABLE_PW_RESET "enable_pw_reset"
#define COLUMN_ENABLE_CONFIRM  "eable_email_verify"
#define COLUMN_COMMON_NAME     "common_name"
#define COLUMN_DISPLAY_NAME    "display_name"
#define COLUMN_USER_ID         "user_id"
#define COLUMN_CHANNEL_NAME    "channel_name"
#define COLUMN_CHANNEL_ID      "channel_id"
#define COLUMN_ACTIVE_UPDATE   "active_updates"
#define COLUMN_SUB_CH_NAME     "sub_channel_name"
#define COLUMN_SUB_CH_ID       "sub_channel_id"
#define COLUMN_PENDING_INVITE  "pending_invite"
#define COLUMN_LOWEST_LEVEL    "lowest_access_level"
#define COLUMN_ACCESS_LEVEL    "access_level"
#define COLUMN_MAX_SUB_CH      "max_sub_channels"
#define COLUMN_APP_NAME        "client_app"
#define COLUMN_DEFAULT_PASS    "default_password"

QString    genPw();
QList<int> genSequence(int min, int max, int len);
QChar      genLetter();
QChar      genNum();
QChar      genSpecialChar();
int        inRange(int pos, int min, int max);
QString    sqlDataPath();
QString    columnType(const QString &column);
quint32    initHostRank();
QByteArray getSalt(const QByteArray &uId, const QString &table);
QByteArray genUniqueHash();
bool       createUser(const QString &userName, const QString &email, const QString &dispName, const QString &password);
bool       createTempPw(const QByteArray &uId, const QString &password);
bool       updatePassword(const QByteArray &uId, const QString &password, const QString &table, bool requireNewPass = false);
bool       auth(const QByteArray &uId, const QString &password, const QString &table);
void       cleanupDbConnection();
void       moveCharLeft(int pos, QString &str);
void       moveCharRight(int pos, QString &str);

class Query : public QObject
{
    Q_OBJECT

public:

    enum QueryType
    {
        UPDATE,
        PUSH,
        PULL,
        DEL,
        INNER_JOIN_PULL,
        CREATE_TABLE,
        ALTER_TABLE
    };

    enum Condition
    {
        EQUAL,
        NOT_EQUAL,
        LIKE,
        LIKE_STARTS_WITH,
        LIKE_ENDS_WITH
    };

    enum FKAction
    {
        NO_ACTION,
        RESTRICT,
        SET_NULL,
        SET_DEFAULT,
        CASCADE
    };

    static QString      getConnectionName();
    static QSqlDatabase getDatabase();

    explicit Query(QObject *parent = nullptr);

    void                     addRandBlob(const QString &column, int len);
    void                     addCondition(const QString &column, const QVariant &data, Condition cond = EQUAL, const QString &tbl = QString());
    void                     addJoinCondition(const QString &column, const QString &joinTable, Condition cond = EQUAL);
    void                     addUnique(const QString &column);
    void                     setPrimary(const QString &column);
    void                     setPrimaryAsc(const QString &column);
    void                     setPrimaryDesc(const QString &column);
    void                     setQueryLimit(uint value, uint offset = 0);
    void                     increment(const QString &column, double value);
    void                     decrement(const QString &column, double value);
    void                     addForeign(const QString &column, const QString &refTable, const QString &refColum, FKAction onDel = RESTRICT, FKAction onUpdate = RESTRICT);
    void                     addColumn(const QString &column);
    void                     addColumn(const QString &column, const QVariant &dataIn);
    void                     addTableColumn(const QString &table, const QString &column);
    void                     setType(QueryType qType, const QString &tbl);
    void                     enableForeignKeys(bool state);
    void                     setTextEncoding(const QString &encoding);
    void                     clearConditions();
    int                      rows();
    int                      columns();
    bool                     exec();
    bool                     createExecuted();
    bool                     inErrorstate();
    QStringList              tables();
    QStringList              columnsInTable(const QString &tbl);
    QVariant                 getData(const QString &column, int row = 0);
    QString                  errDetail();
    QList<QList<QVariant> > &allData();

private:

    bool                    createRan;
    bool                    restraintAdded;
    bool                    queryOk;
    int                     rowsAffected;
    QString                 table;
    QString                 lastErr;
    QString                 limit;
    QString                 qStr;
    QString                 wStr;
    QString                 jStr;
    QueryType               type;
    QList<int>              directBind;
    QStringList             columnsAsPassed;
    QList<QString>          columnList;
    QList<QVariant>         bindValues;
    QList<QVariant>         whereBinds;
    QList<QList<QVariant> > data;

    bool createRedirect();
    void postUpdate();
    void preExec();
    void changeValue(const QString &column, double value, const QString &sign);
};

#endif // DB_H
