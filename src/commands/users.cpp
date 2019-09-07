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
    setParams(TABLE_USERS, QStringList() << COLUMN_TIME << COLUMN_USERNAME << COLUMN_GRNAME << COLUMN_USER_ID, false);
}

LockUser::LockUser(QObject *parent)                           : InternCommand(parent) {}
CreateUser::CreateUser(QObject *parent)                       : InternCommand(parent) {}
RemoveUser::RemoveUser(QObject *parent)                       : InternCommand(parent) {}
ChangeGroup::ChangeGroup(QObject *parent)                     : InternCommand(parent) {}
ChangePassword::ChangePassword(QObject *parent)               : InternCommand(parent) {}
ChangeDispName::ChangeDispName(QObject *parent)               : InternCommand(parent) {}
ChangeUsername::ChangeUsername(QObject *parent)               : InternCommand(parent) {}
OverWriteEmail::OverWriteEmail(QObject *parent)               : InternCommand(parent) {}
ChangeEmail::ChangeEmail(QObject *parent)                     : OverWriteEmail(parent) {}
PasswordChangeRequest::PasswordChangeRequest(QObject *parent) : InternCommand(parent) {}
NameChangeRequest::NameChangeRequest(QObject *parent)         : InternCommand(parent) {}

QString ListUsers::cmdName()             {return "ls_users";}
QString LockUser::cmdName()              {return "lock_acct";}
QString CreateUser::cmdName()            {return "add_acct";}
QString RemoveUser::cmdName()            {return "rm_acct";}
QString ChangeGroup::cmdName()           {return "set_group";}
QString ChangePassword::cmdName()        {return "set_pw";}
QString ChangeDispName::cmdName()        {return "set_disp_name";}
QString ChangeUsername::cmdName()        {return "set_user_name";}
QString OverWriteEmail::cmdName()        {return "force_set_email";}
QString ChangeEmail::cmdName()           {return "set_email";}
QString PasswordChangeRequest::cmdName() {return "request_new_pw";}
QString NameChangeRequest::cmdName()     {return "request_new_user_name";}

void LockUser::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 4);
        QString     uName = getParam("-user", args);
        QString     state = getParam("-state", args);

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
        else if (noCaseMatch(ROOT_USER, uName))
        {
            errTxt("err: Unable to lock/unlock protected user: '" + QString(ROOT_USER) + "'\n");
        }
        else if (!isBool(state))
        {
            errTxt("err: The state bool value (-state) must be a 0 or 1.\n");
        }
        else if (!userExists(uName))
        {
            errTxt("err: The requested user name does not exists.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, getUserGroup(uName)))
        {
            errTxt("err: The target user account out ranks or is equal to your own rank. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_LOCKED, static_cast<bool>(state.toInt()));
            db.addCondition(COLUMN_USERNAME, uName);
            db.exec();
        }
    }
}

void CreateUser::term()
{
    emit enableMoreInput(false);

    email.clear();
    newName.clear();
}

void CreateUser::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        if (moreInputEnabled())
        {
            QString password = fromTEXT(binIn);

            if (password.isEmpty())
            {
                term();
            }
            else if (!validPassword(password))
            {
                errTxt("err: Invalid password. it must be 8-200 chars long containing numbers, mixed case letters and special chars.\n\n");
                privTxt("Enter a new password (leave blank to cancel): ");
            }
            else if (!createUser(newName, email, dispName, password))
            {
                errTxt("err: The requested User name already exists.\n");
                term();
            }
            else
            {
                term();
            }
        }
        else
        {
            QStringList args = parseArgs(binIn, 4);

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
                emit enableMoreInput(true);

                privTxt("Enter a new password (leave blank to cancel): ");
            }
        }
    }
}

void RemoveUser::term()
{
    emit enableMoreInput(false);

    uName.clear();
}

void RemoveUser::rm()
{
    Query db;

    db.setType(Query::DEL, TABLE_USERS);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    emit backendDataOut(ASYNC_USER_DELETED, toTEXT(uName), PUB_IPC_WITH_FEEDBACK);

    term();
}

void RemoveUser::ask()
{
    emit enableMoreInput(true);

    mainTxt("Are you sure you want to permanently remove this user account? (y/n): ");
}

void RemoveUser::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        if (moreInputEnabled())
        {
            QString ans = fromTEXT(binIn);

            if (noCaseMatch("y", ans))
            {
                rm();
            }
            else if (noCaseMatch("n", ans))
            {
                term();
            }
            else
            {
                ask();
            }
        }
        else
        {
            QStringList args = parseArgs(binIn, 2);

            uName = getParam("-name", args);

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
            else if (!userExists(uName))
            {
                errTxt("err: The requested user name does not exists.\n");
            }
            else if (isChOwner(uName))
            {
                errTxt("err: The requested user name is the owner of one or more channels.\n");
            }
            else if (!checkRank(*sharedObjs->groupName, getUserGroup(uName)) && !noCaseMatch(*sharedObjs->userName, uName))
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

void ChangeGroup::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 4);

        QString uName = getParam("-user", args);
        QString gName = getParam("-group", args);

        if (uName.isEmpty())
        {
            errTxt("err: User name argument -user not found or is empty.\n");
        }
        else if (gName.isEmpty())
        {
            errTxt("err: Group name argument -group not found or is empty.\n");
        }
        else if (noCaseMatch(ROOT_USER, uName))
        {
            errTxt("err: You are not allowed to change the group of protected user: '" + QString(ROOT_USER) + "'\n");
        }
        else if (noCaseMatch(ROOT_USER, gName))
        {
            errTxt("err: No user created account is allowed to attach to protected group: '" + QString(ROOT_USER) + "'\n");
        }
        else if (!validGroupName(gName))
        {
            errTxt("err: Invalid group name.\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid username.\n");
        }
        else if (!userExists(uName))
        {
            errTxt("err: The requested user account does not exists.\n");
        }
        else if (!groupExists(gName))
        {
            errTxt("err: The requested group does not exists.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, gName, true))
        {
            errTxt("err: The target group out ranks your own group. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_GRNAME, gName);
            db.addCondition(COLUMN_USERNAME, uName);
            db.exec();

            emit backendDataOut(ASYNC_USER_GROUP_CHANGED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void ChangePassword::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        if (moreInputEnabled())
        {
            QString password = fromTEXT(binIn);

            if (password.isEmpty())
            {
                emit enableMoreInput(false);
            }
            else if (!validPassword(password))
            {
                errTxt("err: Invalid password. it must be 8-200 chars long containing numbers, mixed case letters and special chars.\n\n");
                privTxt("Enter a new password (leave blank to cancel): ");
            }
            else
            {   
                emit enableMoreInput(false);

                updatePassword(*sharedObjs->userName, password, TABLE_USERS);
            }
        }
        else
        {
            emit enableMoreInput(true);

            privTxt("Enter a new password (leave blank to cancel): ");
        }
    }
}

void ChangeUsername::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
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
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_USERNAME, newName);
            db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
            db.exec();

            args.append("-old");
            args.append("'" + escapeChars(*sharedObjs->userName, '\\', '\'') + "'");

            emit backendDataOut(ASYNC_USER_RENAMED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void ChangeDispName::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 2);
        QString     name = getParam("-name", args);

        if (name.isEmpty())
        {
            errTxt("err: New display name argument (-name) not found or is empty.\n");
        }
        else if (!validDispName(name))
        {
            errTxt("err: The display name is too large or contains a newline char. limit: 32 chars.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_DISPLAY_NAME, name);
            db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
            db.exec();

            args.append("-user");
            args.append("'" + escapeChars(*sharedObjs->userName, '\\', '\'') + "'");

            emit backendDataOut(ASYNC_DISP_RENAMED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void OverWriteEmail::procArgs(const QString &uName, const QString &newEmail, bool sameRank, const SharedObjs *sharedObjs)
{
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
    else if (!userExists(uName))
    {
        errTxt("err: The requested user account does not exists.\n");
    }
    else if (!checkRank(*sharedObjs->groupName, getUserGroup(uName), sameRank))
    {
        errTxt("err: The target user account out ranks your own rank. access denied.\n");
    }
    else
    {
        Query db(this);

        db.setType(Query::UPDATE, TABLE_USERS);
        db.addColumn(COLUMN_EMAIL, newEmail);
        db.addColumn(COLUMN_EMAIL_VERIFIED, false);
        db.addCondition(COLUMN_USERNAME, uName);
        db.exec();

        emit backendDataOut(ASYNC_RW_MY_INFO, toTEXT(uName), PUB_IPC_WITH_FEEDBACK);
    }
}

void OverWriteEmail::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args     = parseArgs(binIn, 4);
        QString     uName    = getParam("-user", args);
        QString     newEmail = getParam("-new_email", args);

        procArgs(uName, newEmail, false, sharedObjs);
    }
}

void ChangeEmail::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args     = parseArgs(binIn, 2);
        QString     newEmail = getParam("-new_email", args);

        procArgs(*sharedObjs->userName, newEmail, true, sharedObjs);
    }
}

void PasswordChangeRequest::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 4);
        QString     uName = getParam("-user", args);
        QString     req   = getParam("-req", args);

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
        else if (!userExists(uName))
        {
            errTxt("err: The requested user account does not exists.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, getUserGroup(uName)))
        {
            errTxt("err: The target user account out ranks or is equal to your own rank. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_NEED_PASS, static_cast<bool>(req.toInt()));
            db.addCondition(COLUMN_USERNAME, uName);
            db.exec();
        }
    }
}

void NameChangeRequest::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 4);
        QString     uName = getParam("-user", args);
        QString     req   = getParam("-req", args);

        if (uName.isEmpty())
        {
            errTxt("err: User name (-user) argument was not found or is empty.\n");
        }
        else if (req.isEmpty())
        {
            errTxt("err: Request bool (-req) argument was not found or is empty.\n");
        }
        else if (!isBool(req))
        {
            errTxt("err: The request bool value (-req) must be a 0 or 1.\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid user name.\n");
        }
        else if (!userExists(uName))
        {
            errTxt("err: The requested user account does not exists.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, getUserGroup(uName)))
        {
            errTxt("err: The target user account out ranks or is equal to your own rank. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_NEED_NAME, static_cast<bool>(req.toInt()));
            db.addCondition(COLUMN_USERNAME, uName);
            db.exec();
        }
    }
}
