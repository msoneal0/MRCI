#include "admin.h"

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

CloseHost::CloseHost(QObject *parent)       : CmdObject(parent) {}
RestartHost::RestartHost(QObject *parent)   : CmdObject(parent) {}
ServSettings::ServSettings(QObject *parent) : CmdObject(parent) {}

QString CloseHost::cmdName()    {return "close_host";}
QString RestartHost::cmdName()  {return "restart_host";}
QString ServSettings::cmdName() {return "host_config";}

void CloseHost::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            auto input = fromTEXT(binIn);

            if (input == "CLOSE")
            {
                flags &= ~MORE_INPUT;

                async(ASYNC_EXIT);
            }
            else if (input.isEmpty())
            {
                retCode = ABORTED;
                flags  &= ~MORE_INPUT;
            }
            else
            {
                errTxt("err: Invalid response. you need to type 'CLOSE' exactly as shown without the quotes.\n");
                promptTxt("Enter 'CLOSE' to proceed or leave blank to cancel: ");
            }
        }
        else
        {
            flags |= MORE_INPUT;

            promptTxt("You are about to shutdown the host instance, type: 'CLOSE' to proceed or leave blank to cancel: ");
        }
    }
}

void RestartHost::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            auto input = fromTEXT(binIn);

            if (input == "RESTART")
            {
                flags &= ~MORE_INPUT;

                async(ASYNC_RESTART);
            }
            else if (input.isEmpty())
            {
                retCode = ABORTED;
                flags  &= ~MORE_INPUT;
            }
            else if (!input.isEmpty())
            {
                errTxt("err: Invalid response. you need to type 'RESTART' exactly as shown without the quotes.\n");
                promptTxt("Enter 'RESTART' to proceed or leave blank to cancel: ");
            }
        }
        else
        {
            flags |= MORE_INPUT;

            promptTxt("You are about to re-start the host instance, type: 'RESTART' to proceed or leave blank to cancel: ");
        }
    }
}

void ServSettings::printSettings()
{
    Query db(this);

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_PUB_USERS);
    db.addColumn(COLUMN_LOCK_LIMIT);
    db.addColumn(COLUMN_MAXSESSIONS);
    db.addColumn(COLUMN_INITRANK);
    db.addColumn(COLUMN_MAILERBIN);
    db.addColumn(COLUMN_MAIL_SEND);
    db.addColumn(COLUMN_ENABLE_CONFIRM);
    db.addColumn(COLUMN_ENABLE_PW_RESET);
    db.addColumn(COLUMN_ACTIVE_UPDATE);
    db.addColumn(COLUMN_MAX_SUB_CH);
    db.exec();

    auto pubBool = boolStr(db.getData(COLUMN_PUB_USERS).toBool());
    auto resBool = boolStr(db.getData(COLUMN_ENABLE_PW_RESET).toBool());
    auto conBool = boolStr(db.getData(COLUMN_ENABLE_CONFIRM).toBool());
    auto actBool = boolStr(db.getData(COLUMN_ACTIVE_UPDATE).toBool());

    QString     txt;
    QTextStream txtOut(&txt);

    txtOut << "All Sub-Channels Active Update: " << actBool                                 << Qt::endl;
    txtOut << "Public Registration:            " << pubBool                                 << Qt::endl;
    txtOut << "Automated Password Resets:      " << resBool                                 << Qt::endl;
    txtOut << "Automated Email Verify:         " << conBool                                 << Qt::endl;
    txtOut << "Maximum Sessions:               " << db.getData(COLUMN_MAXSESSIONS).toUInt() << Qt::endl;
    txtOut << "Autolock Threshold:             " << db.getData(COLUMN_LOCK_LIMIT).toUInt()  << Qt::endl;
    txtOut << "Maximum Sub-Channels:           " << db.getData(COLUMN_MAX_SUB_CH).toUInt()  << Qt::endl;
    txtOut << "Initial Host Rank:              " << db.getData(COLUMN_INITRANK).toUInt()    << Qt::endl;
    txtOut << "Root User:                      " << getUserName(rootUserId())               << Qt::endl;
    txtOut << "Working Path:                   " << QDir::currentPath()                     << Qt::endl;
    txtOut << "Database:                       " << sqlDataPath()                           << Qt::endl;
    txtOut << "Mailer Executable:              " << db.getData(COLUMN_MAILERBIN).toString() << Qt::endl;
    txtOut << "Mailer Command:                 " << db.getData(COLUMN_MAIL_SEND).toString() << Qt::endl << Qt::endl;

    mainTxt(txt);
}

void ServSettings::printOptions()
{
    if (level == 0)
    {
        QString     txt;
        QTextStream txtOut(&txt);

        txtOut << "[01] Autolock Threshold  [02] Max Sessions"     << Qt::endl;
        txtOut << "[03] Public Registration [04] Initial Rank"     << Qt::endl;
        txtOut << "[05] Mailer Exe          [06] Mailer Command"   << Qt::endl;
        txtOut << "[07] Password Resets     [08] Email Verify"     << Qt::endl;
        txtOut << "[09] Active Update       [10] Max Sub-Channels" << Qt::endl;
        txtOut << "[11] Set Root User       [00] Exit"             << Qt::endl << Qt::endl;
        txtOut << "Select an option: ";

        level = 1;

        promptTxt(txt);
    }
}

void ServSettings::returnToStart()
{
    select = 0;
    level  = 0;

    mainTxt("\n");
    printSettings();
    printOptions();
}

void ServSettings::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            if (level == 1)
            {
                QString     txt;
                QTextStream txtOut(&txt);

                auto ok = false;

                select = fromTEXT(binIn).toInt(&ok);

                if ((select == 1) && ok)
                {
                    txtOut << ""                                                                    << Qt::endl;
                    txtOut << "The autolock threshold is an integer value that determines how many" << Qt::endl;
                    txtOut << "failed login attempts can be made before the user account is locked" << Qt::endl;
                    txtOut << "by the host."                                                        << Qt::endl << Qt::endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 2) && ok)
                {
                    txtOut << ""                                                                       << Qt::endl;
                    txtOut << "Max sessions is an integar value that determines how many simultaneous" << Qt::endl;
                    txtOut << "clients the host will be allowed to run at once."                       << Qt::endl << Qt::endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 3) && ok)
                {
                    txtOut << ""                                                                     << Qt::endl;
                    txtOut << "Public registration basically allows un-logged in clients to run the" << Qt::endl;
                    txtOut << "add_acct command. doing this allows un-registered users to become"    << Qt::endl;
                    txtOut << "registered users without the need to contact an admin."               << Qt::endl << Qt::endl;
                    txtOut << "[0] Disable"                                                          << Qt::endl;
                    txtOut << "[1] Enable"                                                           << Qt::endl << Qt::endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 4) && ok)
                {
                    txtOut << ""                                                                       << Qt::endl;
                    txtOut << "The initial host rank is the rank all new user accounts are registered" << Qt::endl;
                    txtOut << "with when created. the host rank itself is an integer value that"       << Qt::endl;
                    txtOut << "determine what commands each user can or cannot run."                   << Qt::endl << Qt::endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 5) && ok)
                {
                    txtOut << ""                                                              << Qt::endl;
                    txtOut << "This is the path to the command line email client executable"  << Qt::endl;
                    txtOut << "that the host can utilize to send emails to registered users." << Qt::endl << Qt::endl;
                    txtOut << "note: the host assumes the email application already has a"    << Qt::endl;
                    txtOut << "      configured sender email address/server."                 << Qt::endl << Qt::endl;
                    txtOut << "Enter a new path (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 6) && ok)
                {
                    txtOut << ""                                                                                       << Qt::endl;
                    txtOut << "This is the command line that will be used with the email client"                       << Qt::endl;
                    txtOut << "executable to send emails to registered users. it must contain the"                     << Qt::endl;
                    txtOut << "keywords " << SUBJECT_SUB << ", " << TARGET_EMAIL_SUB << " and " << MSG_SUB << " to be" << Qt::endl;
                    txtOut << "acceptable. the host will substitute these keywords for actual"                         << Qt::endl;
                    txtOut << "parameters when calling the email client."                                              << Qt::endl << Qt::endl;
                    txtOut << "Enter a new command line (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 7) && ok)
                {
                    txtOut << ""                                                              << Qt::endl;
                    txtOut << "This enables automated password resets via email so users can" << Qt::endl;
                    txtOut << "reset their account passwords without the need to contact an"  << Qt::endl;
                    txtOut << "admin. this basically tells the host if it is allowed to load" << Qt::endl;
                    txtOut << "the request_pw_reset and recover_acct commands or not."        << Qt::endl << Qt::endl;
                    txtOut << "[0] Disable"                                                   << Qt::endl;
                    txtOut << "[1] Enable"                                                    << Qt::endl << Qt::endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 8) && ok)
                {
                    txtOut << ""                                                          << Qt::endl;
                    txtOut << "This enables/disables automated email confirmations. this" << Qt::endl;
                    txtOut << "tells the host if it is allowed to load the verify_email " << Qt::endl;
                    txtOut << "command for any user, regardless of rank."                 << Qt::endl << Qt::endl;
                    txtOut << "[0] Disable"                                               << Qt::endl;
                    txtOut << "[1] Enable"                                                << Qt::endl << Qt::endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 9) && ok)
                {
                    txtOut << ""                                                                       << Qt::endl;
                    txtOut << "This option tells the host if all sub-channels should be considered"    << Qt::endl;
                    txtOut << "active or not. otherwise, the active flag can be toggled on/off at the" << Qt::endl;
                    txtOut << "sub-channel level. active sub-channels send/receive PEER_INFO or"       << Qt::endl;
                    txtOut << "PEER_STAT frames with each other so all peers connected to the"         << Qt::endl;
                    txtOut << "sub-channel can be made aware of each other's public information."      << Qt::endl;
                    txtOut << "without the active flag, no such frames are transffered."               << Qt::endl << Qt::endl;
                    txtOut << "[0] Disable"                                                            << Qt::endl;
                    txtOut << "[1] Enable"                                                             << Qt::endl << Qt::endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 10) && ok)
                {
                    txtOut << ""                                                                     << Qt::endl;
                    txtOut << "This option sets the maximum amount of sub-channels each channel can" << Qt::endl;
                    txtOut << "have. the hard maximum is 256 and the minimum is 1."                  << Qt::endl << Qt::endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 11) && ok)
                {
                    txtOut << ""                                                                    << Qt::endl;
                    txtOut << "Set the root user of the host by the given user name. the root user" << Qt::endl;
                    txtOut << "is an unrestricted user that can do anything on the host. this user" << Qt::endl;
                    txtOut << "however is unable to change rank (1) and cannot get deleted. only"   << Qt::endl;
                    txtOut << "the current root user can use this option to appoint an existing"    << Qt::endl;
                    txtOut << "user as the new root."                                               << Qt::endl << Qt::endl;

                    if (rdFromBlock(userId, BLKSIZE_USER_ID) != rootUserId())
                    {
                        txtOut << "Enter a new user name (leave blank to cancel): ";
                    }
                    else
                    {
                        txtOut << "You are not the current root user so this option is blocked." << Qt::endl;
                        txtOut << "Press enter to return to the main menu.";
                    }

                    level = 2;
                }
                else if ((select == 0) && ok)
                {
                    flags &= ~MORE_INPUT;
                }
                else
                {
                    txtOut << "" << Qt::endl << "Select an option: ";
                }

                promptTxt(txt);
            }
            else if (level == 2)
            {
                auto value = fromTEXT(binIn);

                if (value.isEmpty())
                {
                    mainTxt("\n");
                    returnToStart();
                }
                else
                {
                    if ((select == 1) || (select == 2) || (select == 4))
                    {
                        bool    ok;
                        quint32 num = value.toUInt(&ok, 10);

                        if (!ok)
                        {
                            errTxt("err: Invalid 32bit unsigned integer. valid range: 1-4294967295.\n");
                            promptTxt("Enter a new value (leave blank to cancel): ");
                        }
                        else if (num == 0)
                        {
                            errTxt("err: This value cannot be 0, valid range: 1-4294967295.\n");
                            promptTxt("Enter a new value (leave blank to cancel): ");
                        }
                        else
                        {
                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);

                            if      (select == 1) db.addColumn(COLUMN_LOCK_LIMIT, num);
                            else if (select == 2) db.addColumn(COLUMN_MAXSESSIONS, num);
                            else                  db.addColumn(COLUMN_INITRANK, num);

                            db.exec();

                            if (select == 2)
                            {
                                async(ASYNC_MAXSES, wrInt(num, BLKSIZE_HOST_LOAD * 8));
                            }

                            returnToStart();
                        }
                    }
                    else if ((select == 3) || (select == 7) || (select == 8) || (select == 9))
                    {
                        if (!isBool(value))
                        {
                            errTxt("err: Invalid boolean value. must be 0 (false) or 1 (true).\n");
                            promptTxt("Select an option (leave blank to cancel): ");
                        }
                        else
                        {
                            QString column;

                            if      (select == 3) column = COLUMN_PUB_USERS;
                            else if (select == 7) column = COLUMN_ENABLE_PW_RESET;
                            else if (select == 8) column = COLUMN_ENABLE_CONFIRM;
                            else                  column = COLUMN_ACTIVE_UPDATE;

                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                            db.addColumn(column, static_cast<bool>(value.toUInt()));
                            db.exec();

                            returnToStart();
                        }
                    }
                    else if (select == 5)
                    {
                        if (!QFile::exists(expandEnvVariables(value)))
                        {
                            errTxt("err: The given file: '" + value + "' does not exists.\n");
                            promptTxt("Enter a new path (leave blank to cancel): ");
                        }
                        else
                        {
                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                            db.addColumn(COLUMN_MAILERBIN, value);
                            db.exec();

                            returnToStart();
                        }
                    }
                    else if (select == 6)
                    {
                        if (!value.contains(SUBJECT_SUB, Qt::CaseInsensitive))
                        {
                            errTxt("err: The '" + QString(SUBJECT_SUB) + "' keyword is missing.\n");
                            promptTxt("Enter a new command line (leave blank to cancel): ");
                        }
                        else if (!value.contains(TARGET_EMAIL_SUB, Qt::CaseInsensitive))
                        {
                            errTxt("err: The '" + QString(TARGET_EMAIL_SUB) + "' keyword is missing.\n");
                            promptTxt("Enter a new command line (leave blank to cancel): ");
                        }
                        else if (!value.contains(MSG_SUB, Qt::CaseInsensitive))
                        {
                            errTxt("err: The '" + QString(MSG_SUB) + "' keyword is missing.\n");
                            promptTxt("Enter a new command line (leave blank to cancel): ");
                        }
                        else
                        {
                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                            db.addColumn(COLUMN_MAIL_SEND, value);
                            db.exec();

                            returnToStart();
                        }
                    }
                    else if (select == 10)
                    {
                        if (!isInt(value))
                        {
                            errTxt("err: '" + value + "' is not a valid integer.\n");
                            promptTxt("Enter a new value (leave blank to cancel): ");
                        }
                        else if ((value.toInt() < 1) || (value.toInt() > 256))
                        {
                            errTxt("err: A valid maximum sub-channels value ranges between 1-256.\n");
                            promptTxt("Enter a new value (leave blank to cancel): ");
                        }
                        else
                        {
                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                            db.addColumn(COLUMN_MAX_SUB_CH, value.toInt());
                            db.exec();

                            returnToStart();
                        }
                    }
                    else if (select == 11)
                    {
                        QByteArray uId;

                        if (rdFromBlock(userId, BLKSIZE_USER_ID) != rootUserId())
                        {
                            returnToStart();
                        }
                        else if (!validUserName(value))
                        {
                            errTxt("err: Invalid user name. it must be 2-24 chars long and contain no spaces.\n");
                            promptTxt("Enter a new user name (leave blank to cancel): ");
                        }
                        else if (!userExists(value, &uId))
                        {
                            errTxt("err: The requested user name does not exists.\n");
                            promptTxt("Enter a new user name (leave blank to cancel): ");
                        }
                        else
                        {
                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                            db.addColumn(COLUMN_ROOT_USER, value);
                            db.exec();

                            returnToStart();
                        }
                    }
                }
            }
        }
        else
        {
            select = 0;
            level  = 0;
            flags |= MORE_INPUT;

            printSettings();
            printOptions();
        }
    }
}
