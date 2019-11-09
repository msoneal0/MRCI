#include "auth.h"

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

Auth::Auth(QObject *parent) : CmdObject(parent) {}

AuthLog::AuthLog(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_AUTH_LOG, false);
    addJointColumn(TABLE_USERS, COLUMN_USER_ID);
    addTableColumn(TABLE_AUTH_LOG, COLUMN_TIME);
    addTableColumn(TABLE_AUTH_LOG, COLUMN_IPADDR);
    addTableColumn(TABLE_USERS, COLUMN_USERNAME);
    addTableColumn(TABLE_AUTH_LOG, COLUMN_AUTH_ATTEMPT);
    addTableColumn(TABLE_AUTH_LOG, COLUMN_RECOVER_ATTEMPT);
    addTableColumn(TABLE_AUTH_LOG, COLUMN_COUNT);
    addTableColumn(TABLE_AUTH_LOG, COLUMN_ACCEPTED);
}

QString Auth::cmdName()    {return "auth";}
QString AuthLog::cmdName() {return "ls_auth_log";}

void Auth::addToThreshold()
{
    Query db(this);

    db.setType(Query::PUSH, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_USER_ID, uId);
    db.addColumn(COLUMN_IPADDR, ip);
    db.addColumn(COLUMN_AUTH_ATTEMPT, true);
    db.addColumn(COLUMN_RECOVER_ATTEMPT, false);
    db.addColumn(COLUMN_COUNT, true);
    db.addColumn(COLUMN_ACCEPTED, false);
    db.exec();

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);

    quint32 maxAttempts = 0;
    bool    isRoot      = false;

    if (noCaseMatch(ROOT_USER, uName))
    {
        isRoot = true;

        db.addColumn(COLUMN_BAN_LIMIT);
        db.exec();

        maxAttempts = db.getData(COLUMN_BAN_LIMIT).toUInt();
    }
    else
    {
        db.addColumn(COLUMN_LOCK_LIMIT);
        db.exec();

        maxAttempts = db.getData(COLUMN_LOCK_LIMIT).toUInt();
    }

    db.setType(Query::PULL, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_IPADDR);
    db.addCondition(COLUMN_USER_ID, uId);
    db.addCondition(COLUMN_AUTH_ATTEMPT, true);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_ACCEPTED, false);

    if (isRoot) db.addCondition(COLUMN_IPADDR, ip);

    db.exec();

    if (static_cast<quint32>(db.rows()) > maxAttempts)
    {
        if (isRoot)
        {
            if (!QHostAddress(ip).isLoopback())
            {
                db.setType(Query::PUSH, TABLE_IPBANS);
                db.addColumn(COLUMN_IPADDR, ip);
                db.exec();

                async(ASYNC_UPDATE_BANS, PRIV_IPC);
                async(ASYNC_END_SESSION, PRIV_IPC);
            }
        }
        else
        {
            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_LOCKED, true);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();
        }

        flags &= ~MORE_INPUT;
    }
    else
    {
        errTxt("err: Access denied.\n\n");
        privTxt("Enter password (leave blank to cancel): ");
    }
}

void Auth::confirmAuth()
{
    Query db(this);

    db.setType(Query::UPDATE, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_COUNT, false);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_USER_ID, uId);
    db.addCondition(COLUMN_AUTH_ATTEMPT, true);

    if (noCaseMatch(ROOT_USER, uName))
    {
        db.addCondition(COLUMN_IPADDR, ip);
    }

    db.exec();

    db.setType(Query::PUSH, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_USER_ID, uId);
    db.addColumn(COLUMN_IPADDR, ip);
    db.addColumn(COLUMN_COUNT, false);
    db.addColumn(COLUMN_ACCEPTED, true);
    db.addColumn(COLUMN_AUTH_ATTEMPT, true);
    db.addColumn(COLUMN_RECOVER_ATTEMPT, false);
    db.exec();

    flags &= ~MORE_INPUT;

    async(ASYNC_USER_LOGIN, PRIV_IPC, uId);
    mainTxt("Access granted.\n");
}

void Auth::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            QString text = fromTEXT(binIn);

            if (loginOk)
            {
                if (newPassword)
                {
                    if (text.isEmpty())
                    {
                        mainTxt("\n");

                        flags &= ~MORE_INPUT;
                    }
                    else if (!validPassword(text))
                    {
                        errTxt("err: Invalid password. it must be 8-200 chars long containing numbers, mixed case letters and special chars.\n\n");
                        privTxt("Enter a new password (leave blank to cancel): ");
                    }
                    else
                    {
                        updatePassword(uId, text, TABLE_USERS);

                        Query db(this);

                        db.setType(Query::UPDATE, TABLE_USERS);
                        db.addColumn(COLUMN_NEED_PASS, false);
                        db.addCondition(COLUMN_USER_ID, uId);
                        db.exec();

                        newPassword = false;

                        if (newUserName)
                        {
                            mainTxt("Enter a new user name: ");
                        }
                        else
                        {
                            confirmAuth();
                        }
                    }
                }
                else if (newUserName)
                {
                    if (text.isEmpty())
                    {
                        mainTxt("\n");

                        flags &= ~MORE_INPUT;
                    }
                    else if (!validUserName(text))
                    {
                        errTxt("err: Invalid username. it must be 2-24 chars long and contain no spaces.\n\n");
                        mainTxt("Enter a new user name (leave blank to cancel): ");
                    }
                    else if (!userExists(text))
                    {
                        errTxt("err: The requested User name already exists.\n\n");
                        mainTxt("Enter a new user name (leave blank to cancel): ");
                    }
                    else
                    {
                        Query db(this);

                        db.setType(Query::UPDATE, TABLE_USERS);
                        db.addColumn(COLUMN_NEED_NAME, false);
                        db.addColumn(COLUMN_USERNAME, text);
                        db.addCondition(COLUMN_USER_ID, uId);
                        db.exec();

                        async(ASYNC_USER_RENAMED, PUB_IPC, uId + fixedToTEXT(text, BLKSIZE_USER_NAME));

                        uName       = text;
                        newUserName = false;

                        confirmAuth();
                    }
                }
            }
            else if (text.isEmpty())
            {
                mainTxt("\n");

                flags &= ~MORE_INPUT;
            }
            else if (!validPassword(text))
            {
                addToThreshold();
            }
            else if (!auth(uId, text, TABLE_USERS))
            {
                addToThreshold();
            }
            else
            {
                loginOk = true;

                if (newPassword)
                {
                    privTxt("Enter a new password (leave blank to cancel): ");
                }
                else if (newUserName)
                {
                    mainTxt("Enter a new user name (leave blank to cancel): ");
                }
                else
                {
                    confirmAuth();
                }
            }
        }
        else
        {
            QStringList args  = parseArgs(binIn, 2);
            QString     email = getParam("-email", args);
            QString     name  = getParam("-user", args);

            if (!email.isEmpty() && validEmailAddr(email))
            {
                name = getUserNameForEmail(email);
            }

            if (name.isEmpty() || !validUserName(name))
            {
                errTxt("err: The -user or -email argument is empty, not found or invalid.\n");
            }
            else if (!userExists(name, &uId))
            {
                errTxt("err: No such user.\n");
            }
            else if (isLocked(uId))
            {
                errTxt("err: The requested user account is locked.\n");
            }
            else
            {
                Query db(this);

                db.setType(Query::PULL, TABLE_USERS);
                db.addColumn(COLUMN_NEED_NAME);
                db.addColumn(COLUMN_NEED_PASS);
                db.addColumn(COLUMN_USER_ID);
                db.addCondition(COLUMN_USER_ID, uId);
                db.exec();

                loginOk     = false;
                uName       = name;
                flags      |= MORE_INPUT;
                ip          = rdStringFromBlock(clientIp, BLKSIZE_CLIENT_IP);
                newPassword = db.getData(COLUMN_NEED_PASS).toBool();
                newUserName = db.getData(COLUMN_NEED_NAME).toBool();

                privTxt("Enter password (leave blank to cancel): ");
            }
        }
    }
}
