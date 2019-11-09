#include "users.h"

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

ListUsers::ListUsers(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_USERS, false);
    addTableColumn(TABLE_USERS, COLUMN_TIME);
    addTableColumn(TABLE_USERS, COLUMN_USERNAME);
    addTableColumn(TABLE_USERS, COLUMN_HOST_RANK);
    addTableColumn(TABLE_USERS, COLUMN_USER_ID);
}

LockUser::LockUser(QObject *parent)                           : CmdObject(parent) {}
CreateUser::CreateUser(QObject *parent)                       : CmdObject(parent) {}
RemoveUser::RemoveUser(QObject *parent)                       : CmdObject(parent) {}
ChangeUserRank::ChangeUserRank(QObject *parent)               : CmdObject(parent) {}
ChangePassword::ChangePassword(QObject *parent)               : CmdObject(parent) {}
ChangeDispName::ChangeDispName(QObject *parent)               : CmdObject(parent) {}
ChangeUsername::ChangeUsername(QObject *parent)               : CmdObject(parent) {}
OverWriteEmail::OverWriteEmail(QObject *parent)               : CmdObject(parent) {}
ChangeEmail::ChangeEmail(QObject *parent)                     : OverWriteEmail(parent) {}
PasswordChangeRequest::PasswordChangeRequest(QObject *parent) : CmdObject(parent) {}
NameChangeRequest::NameChangeRequest(QObject *parent)         : PasswordChangeRequest(parent) {}

QString ListUsers::cmdName()             {return "ls_users";}
QString LockUser::cmdName()              {return "lock_acct";}
QString CreateUser::cmdName()            {return "add_acct";}
QString RemoveUser::cmdName()            {return "rm_acct";}
QString ChangeUserRank::cmdName()        {return "set_user_rank";}
QString ChangePassword::cmdName()        {return "set_pw";}
QString ChangeDispName::cmdName()        {return "set_disp_name";}
QString ChangeUsername::cmdName()        {return "set_user_name";}
QString OverWriteEmail::cmdName()        {return "force_set_email";}
QString ChangeEmail::cmdName()           {return "set_email";}
QString PasswordChangeRequest::cmdName() {return "request_new_pw";}
QString NameChangeRequest::cmdName()     {return "request_new_user_name";}

bool canModifyUser(const QByteArray &uId, quint32 myRank, bool equalAcceptable)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_HOST_RANK);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    if (equalAcceptable)
    {
        return myRank <= db.getData(COLUMN_HOST_RANK).toUInt();
    }
    else
    {
        return myRank < db.getData(COLUMN_HOST_RANK).toUInt();
    }
}

void LockUser::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 4);
        QString     uName = getParam("-user", args);
        QString     state = getParam("-state", args);
        QByteArray  uId;

        if (uName.isEmpty())
        {
            errTxt("err: User name (-user) argument not found or is empty.\n");
        }
        else if (state.isEmpty())
        {
            errTxt("err: State (-state) argument not found or is empty.\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid user name.\n");
        }
        else if (!isBool(state))
        {
            errTxt("err: The state bool value (-state) must be a 0 or 1.\n");
        }
        else if (!userExists(uName, &uId))
        {
            errTxt("err: The requested user name does not exists.\n");
        }
        else if (!canModifyUser(uId, rd32BitFromBlock(hostRank), false))
        {
            errTxt("err: The target user account out ranks you or is equal to your own rank. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_LOCKED, static_cast<bool>(state.toInt()));
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();
        }
    }
}

void CreateUser::clear()
{
    flags = 0;

    email.clear();
    newName.clear();
}

void CreateUser::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            QString password = fromTEXT(binIn);

            if (password.isEmpty())
            {
                clear();
            }
            else if (!validPassword(password))
            {
                errTxt("err: Invalid password. it must be 8-200 chars long containing numbers, mixed case letters and special chars.\n\n");
                privTxt("Enter a new password (leave blank to cancel): ");
            }
            else if (!createUser(newName, email, dispName, password))
            {
                errTxt("err: The requested User name already exists.\n");
                clear();
            }
            else
            {
                clear();
            }
        }
        else
        {
            QStringList args = parseArgs(binIn, 6);

            dispName = getParam("-disp", args);
            newName  = getParam("-name", args);
            email    = getParam("-email", args);

            if (newName.isEmpty())
            {
                errTxt("err: Username (-name) argument not found or is empty.\n");
            }
            else if (email.isEmpty())
            {
                errTxt("err: Email (-email) argument not found or is empty.\n");
            }
            else if (!validUserName(newName))
            {
                errTxt("err: Invalid username. it must be 2-24 chars long and contain no spaces.\n");
            }
            else if (!validEmailAddr(email))
            {
                errTxt("err: Invalid email address. it must contain a '@' symbol along with a vaild host address and user name that contain no spaces. it must also be less than 64 chars long.\n");
            }
            else if (!validDispName(dispName))
            {
                errTxt("err: The display name is too large or contains a newline char. char limit: 32.\n");
            }
            else if (userExists(newName))
            {
                errTxt("err: The requested User name already exists.\n");
            }
            else if (emailExists(email))
            {
                errTxt("err: The requested email address is already in use.\n");
            }
            else
            {
                flags |= MORE_INPUT;

                privTxt("Enter a new password (leave blank to cancel): ");
            }
        }
    }
}

void RemoveUser::rm()
{
    Query db;

    db.setType(Query::DEL, TABLE_USERS);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    flags &= ~MORE_INPUT;

    async(ASYNC_USER_DELETED, PUB_IPC_WITH_FEEDBACK, uId);
}

void RemoveUser::ask()
{
    flags |= MORE_INPUT;

    mainTxt("Are you sure you want to permanently remove this user account? (y/n): ");
}

void RemoveUser::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            QString ans = fromTEXT(binIn);

            if (noCaseMatch("y", ans))
            {
                rm();
            }
            else if (noCaseMatch("n", ans))
            {
                flags &= ~MORE_INPUT;
            }
            else
            {
                ask();
            }
        }
        else
        {
            QStringList args  = parseArgs(binIn, 2);
            QString     uName = getParam("-name", args);

            if (uName.isEmpty())
            {
                errTxt("err: User name argument (-name) not found or is empty.\n");
            }
            else if (noCaseMatch(ROOT_USER, uName))
            {
                errTxt("err: Unable to delete protected user: '" + QString(ROOT_USER) + "'\n");
            }
            else if (!validUserName(uName))
            {
                errTxt("err: Invalid username.\n");
            }
            else if (!userExists(uName, &uId))
            {
                errTxt("err: The requested user name does not exists.\n");
            }
            else if (isChOwner(uId))
            {
                errTxt("err: The requested user name is the owner of one or more channels. assign new owners for these channels before attempting to delete this account.\n");
            }
            else if (!canModifyUser(uId, rd32BitFromBlock(hostRank), false) && (rdFromBlock(userId, BLKSIZE_USER_ID) != uId))
            {
                errTxt("err: The target user account out ranks you, access denied.\n");
            }
            else
            {
                if (argExists("-force", args))
                {
                    rm();
                }
                else
                {
                    ask();
                }
            }
        }
    }
}

void ChangeUserRank::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 4);
        QString     uName = getParam("-user", args);
        QString     rank  = getParam("-rank", args);
        QByteArray  uId;

        if (uName.isEmpty())
        {
            errTxt("err: User name argument (-user) not found or is empty.\n");
        }
        else if (rank.isEmpty())
        {
            errTxt("err: New rank argument (-rank) not found or is empty.\n");
        }
        else if (noCaseMatch(ROOT_USER, uName))
        {
            errTxt("err: You are not allowed to change the rank of protected user: '" + QString(ROOT_USER) + "'\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid username.\n");
        }
        else if (!isInt(rank))
        {
            errTxt("err: Invalid 32bit unsigned integer for the new rank.\n");
        }
        else if (rank.toUInt() == 0)
        {
            errTxt("err: Rank 0 is invalid. please set a rank of 1 or higher.\n");
        }
        else if (rank.toUInt() < rd32BitFromBlock(hostRank))
        {
            errTxt("err: You cannot assign a rank higher than your own.\n");
        }
        else if (!userExists(uName, &uId))
        {
            errTxt("err: The requested user account does not exists.\n");
        }
        else if (!canModifyUser(uId, rd32BitFromBlock(hostRank), false))
        {
            errTxt("err: The target user out ranks you or is equal to your own rank. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_HOST_RANK, rank.toUInt());
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            async(ASYNC_USER_RANK_CHANGED, PUB_IPC_WITH_FEEDBACK, uId + wrInt(rank.toUInt(), 32));
        }
    }
}

void ChangePassword::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            QString password = fromTEXT(binIn);

            if (password.isEmpty())
            {
                flags &= ~MORE_INPUT;
            }
            else if (!validPassword(password))
            {
                errTxt("err: Invalid password. it must be 8-200 chars long containing numbers, mixed case letters and special chars.\n\n");
                privTxt("Enter a new password (leave blank to cancel): ");
            }
            else
            {   
                flags &= ~MORE_INPUT;

                updatePassword(rdFromBlock(userId, BLKSIZE_USER_ID), password, TABLE_USERS);
            }
        }
        else
        {
            flags |= MORE_INPUT;

            privTxt("Enter a new password (leave blank to cancel): ");
        }
    }
}

void ChangeUsername::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 2);
        QString     newName = getParam("-new_name", args);

        if (newName.isEmpty())
        {
            errTxt("err: New user name argument (-new_name) not found or is empty.\n");
        }
        else if (!validUserName(newName))
        {
            errTxt("err: Invalid username. it must be 2-24 chars long and contain no spaces.\n");
        }
        else if (userExists(newName))
        {
            errTxt("err: The requested user name already exists.\n");
        }
        else
        {
            QByteArray uId       = rdFromBlock(userId, BLKSIZE_USER_ID);
            QByteArray newNameBa = fixedToTEXT(newName, BLKSIZE_USER_NAME);

            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_USERNAME, newName);
            db.addCondition(COLUMN_USER_ID, rdFromBlock(userId, BLKSIZE_USER_ID));
            db.exec();

            async(ASYNC_USER_RENAMED, PUB_IPC_WITH_FEEDBACK, uId + newNameBa);
        }
    }
}

void ChangeDispName::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 2);
        QString     name = getParam("-new_name", args).trimmed();

        if (argExists("-new_name", args))
        {
            errTxt("err: New display name argument (-new_name) not found.\n");
        }
        else if (!validDispName(name))
        {
            errTxt("err: The display name is too large or contains a newline char. limit: 32 chars.\n");
        }
        else
        {
            Query db(this);

            QByteArray uId       = rdFromBlock(userId, BLKSIZE_USER_ID);
            QByteArray newNameBa = fixedToTEXT(name, BLKSIZE_DISP_NAME);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_DISPLAY_NAME, name);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            async(ASYNC_DISP_RENAMED, PUB_IPC_WITH_FEEDBACK, uId + newNameBa);
        }
    }
}

void OverWriteEmail::procArgs(const QString &uName, const QString &newEmail, bool sameRank)
{
    QByteArray uId;

    if (newEmail.isEmpty())
    {
        errTxt("err: New email address (-new_email) argument was not found or is empty.\n");
    }
    else if (uName.isEmpty())
    {
        errTxt("err: User name (-user) argument was not found or is empty.\n");
    }
    else if (!validUserName(uName))
    {
        errTxt("err: Invalid user name.\n");
    }
    else if (!validEmailAddr(newEmail))
    {
        errTxt("err: Invalid email address.\n");
    }
    else if (emailExists(newEmail))
    {
        errTxt("err: The requested email address is already in use.\n");
    }
    else if (!userExists(uName, &uId))
    {
        errTxt("err: The requested user account does not exists.\n");
    }
    else if (!canModifyUser(uId, rd32BitFromBlock(hostRank), sameRank))
    {
        errTxt("err: Access denied.\n");
    }
    else
    {
        Query db(this);

        db.setType(Query::UPDATE, TABLE_USERS);
        db.addColumn(COLUMN_EMAIL, newEmail);
        db.addColumn(COLUMN_EMAIL_VERIFIED, false);
        db.addCondition(COLUMN_USER_ID, uId);
        db.exec();

        async(ASYNC_RW_MY_INFO, PUB_IPC_WITH_FEEDBACK, uId);
    }
}

void OverWriteEmail::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args     = parseArgs(binIn, 4);
        QString     uName    = getParam("-user", args);
        QString     newEmail = getParam("-new_email", args);

        procArgs(uName, newEmail, false);
    }
}

void ChangeEmail::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args     = parseArgs(binIn, 2);
        QString     newEmail = getParam("-new_email", args);

        procArgs(rdStringFromBlock(userName, BLKSIZE_USER_NAME), newEmail, true);
    }
}

void PasswordChangeRequest::exec(const QByteArray &uId, bool req)
{
    Query db(this);

    db.setType(Query::UPDATE, TABLE_USERS);
    db.addColumn(COLUMN_NEED_PASS, req);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();
}

void PasswordChangeRequest::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 4);
        QString     uName = getParam("-user", args);
        QString     req   = getParam("-req", args);
        QByteArray  uId;

        if (uName.isEmpty())
        {
            errTxt("err: User name (-user) argument is missing or empty.\n");
        }
        else if (req.isEmpty())
        {
            errTxt("err: Request bool (-req) argument is missing or empty.\n");
        }
        else if (!isBool(req))
        {
            errTxt("err: The request bool value (-req) must be a 0 or 1.\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid user name.\n");
        }
        else if (!userExists(uName, &uId))
        {
            errTxt("err: The requested user account does not exists.\n");
        }
        else if (!canModifyUser(uId, rd32BitFromBlock(hostRank), false))
        {
            errTxt("err: The target user account out ranks or is equal to your own rank. access denied.\n");
        }
        else
        {
            exec(uId, static_cast<bool>(req.toUInt()));
        }
    }
}

void NameChangeRequest::exec(const QByteArray &uId, bool req)
{
    Query db(this);

    db.setType(Query::UPDATE, TABLE_USERS);
    db.addColumn(COLUMN_NEED_NAME, req);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();
}
