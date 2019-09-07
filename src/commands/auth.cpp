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

Auth::Auth(QObject *parent) : InternCommand(parent) {}

AuthLog::AuthLog(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_AUTH_LOG, QStringList() << COLUMN_TIME
                                            << COLUMN_IPADDR
                                            << COLUMN_USERNAME
                                            << COLUMN_AUTH_ATTEMPT
                                            << COLUMN_RECOVER_ATTEMPT
                                            << COLUMN_COUNT
                                            << COLUMN_ACCEPTED, true);
}

QString Auth::cmdName()    {return "auth";}
QString AuthLog::cmdName() {return "ls_auth_log";}

void Auth::term()
{
    emit enableMoreInput(false);

    newPassword = false;
    newUserName = false;
    loginOk     = false;

    uName.clear();
}

void Auth::addToThreshold(const SharedObjs *sharedObjs)
{
    Query db(this);

    db.setType(Query::PUSH, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_USERNAME, uName);
    db.addColumn(COLUMN_IPADDR, *sharedObjs->sessionAddr);
    db.addColumn(COLUMN_AUTH_ATTEMPT, true);
    db.addColumn(COLUMN_RECOVER_ATTEMPT, false);
    db.addColumn(COLUMN_COUNT, true);
    db.addColumn(COLUMN_ACCEPTED, false);
    db.exec();

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);

    uint maxAttempts = 0;
    bool isRoot      = false;

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
    db.addCondition(COLUMN_USERNAME, uName);
    db.addCondition(COLUMN_AUTH_ATTEMPT, true);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_ACCEPTED, false);

    if (isRoot) db.addCondition(COLUMN_IPADDR, *sharedObjs->sessionAddr);

    db.exec();

    if (static_cast<uint>(db.rows()) > maxAttempts)
    {
        if (isRoot)
        {
            if (!QHostAddress(*sharedObjs->sessionAddr).isLoopback())
            {
                db.setType(Query::PUSH, TABLE_IPBANS);
                db.addColumn(COLUMN_IPADDR, *sharedObjs->sessionAddr);
                db.exec();

                emit closeSession();
            }
        }
        else
        {
            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_LOCKED, true);
            db.addCondition(COLUMN_USERNAME, uName);
            db.exec();
        }

        term();
    }
    else
    {
        errTxt("err: Access denied.\n\n");
        privTxt("Enter password (leave blank to cancel): ");
    }
}

void Auth::confirmAuth(const SharedObjs *sharedObjs)
{
    *rwSharedObjs->userName    = uName;
    *rwSharedObjs->displayName = dName;
    *rwSharedObjs->userId      = uId;
    *rwSharedObjs->groupName   = getUserGroup(uName);
    *rwSharedObjs->hostRank    = getRankForGroup(*sharedObjs->groupName);

    Query db(this);

    db.setType(Query::UPDATE, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_COUNT, false);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_USERNAME, uName);
    db.addCondition(COLUMN_AUTH_ATTEMPT, true);

    if (noCaseMatch(ROOT_USER, uName))
    {
        db.addCondition(COLUMN_IPADDR, *sharedObjs->sessionAddr);
    }

    db.exec();

    db.setType(Query::PUSH, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_USERNAME, uName);
    db.addColumn(COLUMN_IPADDR, *sharedObjs->sessionAddr);
    db.addColumn(COLUMN_COUNT, false);
    db.addColumn(COLUMN_ACCEPTED, true);
    db.addColumn(COLUMN_AUTH_ATTEMPT, true);
    db.addColumn(COLUMN_RECOVER_ATTEMPT, false);
    db.exec();

    mainTxt("Access granted.\n");

    emit authOk();
}

void Auth::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{   
    if (dType == TEXT)
    {
        if (moreInputEnabled())
        {
            QString text = fromTEXT(binIn);

            if (loginOk)
            {
                if (newPassword)
                {
                    if (text.isEmpty())
                    {
                        mainTxt("\n");
                        term();
                    }
                    else if (!validPassword(text))
                    {
                        errTxt("err: Invalid password. it must be 8-200 chars long containing numbers, mixed case letters and special chars.\n\n");
                        privTxt("Enter a new password (leave blank to cancel): ");
                    }
                    else
                    {
                        updatePassword(uName, text, TABLE_USERS);

                        Query db(this);

                        db.setType(Query::UPDATE, TABLE_USERS);
                        db.addColumn(COLUMN_NEED_PASS, false);
                        db.addCondition(COLUMN_USERNAME, uName);
                        db.exec();

                        newPassword = false;

                        if (newUserName)
                        {
                            mainTxt("Enter a new user name: ");
                        }
                        else
                        {
                            confirmAuth(sharedObjs);
                            term();
                        }
                    }
                }
                else if (newUserName)
                {
                    if (text.isEmpty())
                    {
                        mainTxt("\n");
                        term();
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
                        db.addCondition(COLUMN_USERNAME, uName);
                        db.exec();

                        emit backendDataOut(ASYNC_USER_RENAMED, toTEXT("-old '" + escapeChars(uName, '\\', '\'') + "' -new '" + escapeChars(text, '\\', '\'') + "'"), PUB_IPC);

                        uName       = text;
                        newUserName = false;

                        confirmAuth(sharedObjs);
                        term();
                    }
                }
            }
            else if (text.isEmpty())
            {
                mainTxt("\n");
                term();
            }
            else if (!validPassword(text))
            {
                addToThreshold(sharedObjs);
            }
            else if (!auth(uName, text, TABLE_USERS))
            {
                addToThreshold(sharedObjs);
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
                    confirmAuth(sharedObjs);
                    term();
                }
            }
        }
        else
        {
            QStringList args  = parseArgs(binIn, 2);
            QString     email = getParam("-email", args);
            QString     name  = getParam("-user", args);

            if (!email.isEmpty() && validEmailAddr(email)) name = getUserNameForEmail(email);

            if (name.isEmpty() || !validUserName(name))
            {
                errTxt("err: The -user or -email argument is empty, not found or invalid.\n");
            }
            else if (!userExists(name))
            {
                errTxt("err: No such user.\n");
            }
            else if (isLocked(name))
            {
                errTxt("err: The requested user account is locked.\n");
            }
            else
            {
                emit enableMoreInput(true);

                uName = name;

                Query db(this);

                db.setType(Query::PULL, TABLE_USERS);
                db.addColumn(COLUMN_DISPLAY_NAME);
                db.addColumn(COLUMN_NEED_NAME);
                db.addColumn(COLUMN_NEED_PASS);
                db.addColumn(COLUMN_USER_ID);
                db.addCondition(COLUMN_USERNAME, uName);
                db.exec();

                newPassword = db.getData(COLUMN_NEED_PASS).toBool();
                newUserName = db.getData(COLUMN_NEED_NAME).toBool();
                dName       = db.getData(COLUMN_DISPLAY_NAME).toString();
                uId         = db.getData(COLUMN_USER_ID).toByteArray();

                privTxt("Enter password (leave blank to cancel): ");
            }
        }
    }
}
