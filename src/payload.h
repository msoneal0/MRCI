#ifndef PAYLOAD_H
#define PAYLOAD_H

//    This file is part of MRCI_Host.

//    MRCI_Host is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    MRCI_Host is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with MRCI_Host under the LICENSE.md file. If not, see
//    <http://www.gnu.org/licenses/>.

#include <QList>
#include <QObject>
#include <QtEndian>
#include <QSharedMemory>
#include <QSharedPointer>
#include <QTextStream>

#include "db.h"
#include "commands/command.h"

#define MAX_BITS                   24
#define IPC_CMD_REBIND             "bind"
#define IPC_CMD_EXIT               "exit"
#define IPC_CMD_CAST               "cast"
#define IPC_CMD_MAXSES             "max_sessions_changed"
#define IPC_CMD_LOGOUT             "force_logout"
#define IPC_CMD_GROUP_RENAMED      "group_renamed"
#define IPC_CMD_USER_RENAMED       "user_renamed"
#define IPC_CMD_GROUP_TRANS        "group_transferred"
#define IPC_CMD_USER_GROUP_CHANGED "user_group_changed"
#define IPC_CMD_PERM_IDS_CHANGED   "permission_ids_changed"
#define IPC_CMD_RESTART            "restart"
#define IPC_CMD_ENABLE_MOD         "enable_mod"
#define IPC_CMD_DISABLE_MOD        "disable_mod"
#define IPC_CMD_GROUP_UPDATED      "group_params_changed"
#define IPC_CMD_END_SESSION        "end_Session"

enum PrivateTypeID
{
    PRIV_IPC = 1,
    PUB_IPC  = 2
};

enum ExecutorFlags
{
    IPC_LINK_OK     = 1,
    DSIZE_RDY       = 1 << 1,
    DTYPE_RDY       = 1 << 2,
    ALLOW_PRIV_IPC  = 1 << 3,
    CMD_LOOP        = 1 << 4,
    CMD_PAUSED      = 1 << 5,
    LOGGED_IN       = 1 << 6,
    REBUILD_ON_IDLE = 1 << 7
};

enum SessionFlags
{
    CRASHED_STATE              = 1 << 8,
    SSL_NEEDED                 = 1 << 9,
    VER_OK                     = 1 << 10,
    EXPECTED_TERM              = 1 << 11,
    ACTIVE_PAYLOAD             = 1 << 12,
    END_SESSION_ON_PAYLOAD_DEL = 1 << 13
};

enum ServerFlags
{
    FIRST_START    = 1 << 14,
    RES_ON_EMPTY   = 1 << 15,
    CLOSE_ON_EMPTY = 1 << 16,
    ACCEPTING      = 1 << 17
};

QSharedPointer<QByteArray> toIPCPointer(const QByteArray &data);
QString                    sessionCountShareKey();
QByteArray                 wrFrame(const QByteArray &data, uchar dType);
QByteArray                 wrInt(quint64 num, int numOfBits);
void                       wrInt(QSharedMemory *mem, uint value);
uint                       rdInt(const QByteArray &bytes);
uint                       rdSessionLoad();

class Session;

class Payload : public QObject
{
    Q_OBJECT

public:

    Session *obj;

    explicit Payload(QObject *parent = 0);
};

class InternCommand : public ExternCommand
{
    Q_OBJECT

public:

    QTextStream toServ;

    explicit InternCommand(QObject *parent = 0);

signals:

    void loginSuccess();
};

#endif // PAYLOAD_H
