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
            QString input = fromTEXT(binIn);

            if (input == "CLOSE")
            {
                flags &= ~MORE_INPUT;

                async(ASYNC_EXIT, PRIV_IPC);
            }
            else if (input.isEmpty())
            {
                flags &= ~MORE_INPUT;
            }
            else
            {
                errTxt("err: Invalid response. you need to type 'CLOSE' exactly as shown without the quotes.\n");
                mainTxt("Enter 'CLOSE' to proceed or leave blank to cancel: ");
            }
        }
        else
        {
            flags |= MORE_INPUT;

            mainTxt("You are about to shutdown the host instance, type: 'CLOSE' to proceed or leave blank to cancel: ");
        }
    }
}

void RestartHost::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            QString input = fromTEXT(binIn);

            if (input == "RESTART")
            {
                flags &= ~MORE_INPUT;

                async(ASYNC_RESTART, PRIV_IPC);
            }
            else if (input.isEmpty())
            {
                flags &= ~MORE_INPUT;
            }
            else if (!input.isEmpty())
            {
                errTxt("err: Invalid response. you need to type 'RESTART' exactly as shown without the quotes.\n");
                mainTxt("Enter 'RESTART' to proceed or leave blank to cancel: ");
            }
        }
        else
        {
            flags |= MORE_INPUT;

            mainTxt("You are about to re-start the host instance, type: 'RESTART' to proceed or leave blank to cancel: ");
        }
    }
}

void ServSettings::printSettings()
{
    Query db(this);

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_PUB_USERS);
    db.addColumn(COLUMN_BAN_LIMIT);
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

    QString pubBool = boolStr(db.getData(COLUMN_PUB_USERS).toBool());
    QString resBool = boolStr(db.getData(COLUMN_ENABLE_PW_RESET).toBool());
    QString conBool = boolStr(db.getData(COLUMN_ENABLE_CONFIRM).toBool());
    QString actBool = boolStr(db.getData(COLUMN_ACTIVE_UPDATE).toBool());

    QString     txt;
    QTextStream txtOut(&txt);

    txtOut << "All Sub-Channels Active Update: " << actBool                                   << endl;
    txtOut << "Public Registration:            " << pubBool                                   << endl;
    txtOut << "Automated Password Resets:      " << resBool                                   << endl;
    txtOut << "Automated Email Verify:         " << conBool                                   << endl;
    txtOut << "Maximum Sessions:               " << db.getData(COLUMN_MAXSESSIONS).toUInt()   << endl;
    txtOut << "Autoban Threshold:              " << db.getData(COLUMN_BAN_LIMIT).toUInt()     << endl;
    txtOut << "Autolock Threshold:             " << db.getData(COLUMN_LOCK_LIMIT).toUInt()    << endl;
    txtOut << "Maximum Sub-Channels:           " << db.getData(COLUMN_MAX_SUB_CH).toUInt()    << endl;
    txtOut << "Initial Host Rank:              " << db.getData(COLUMN_INITRANK).toUInt()      << endl;
    txtOut << "Database Path:                  " << sqlDataPath()                             << endl;
    txtOut << "Mailer Executable:              " << db.getData(COLUMN_MAILERBIN).toString()   << endl;
    txtOut << "Mailer Command:                 " << db.getData(COLUMN_MAIL_SEND).toString()   << endl << endl;

    mainTxt(txt);
}

void ServSettings::printOptions()
{
    if (level == 0)
    {
        QString     txt;
        QTextStream txtOut(&txt);

        txtOut << "[01] Autoban Threshold  [02] Autolock Threshold"  << endl;
        txtOut << "[03] Max Sessions       [04] Public Registration" << endl;
        txtOut << "[05] Initial Rank       [06] Mailer Exe"          << endl;
        txtOut << "[07] Mailer Command     [08] Password Resets"     << endl;
        txtOut << "[09] Email Verify       [10] Active Update"       << endl;
        txtOut << "[11] Max Sub-Channels   [00] Exit"                << endl << endl;
        txtOut << "Select an option: ";

        level = 1;

        mainTxt(txt);
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

                bool ok = false;

                select = fromTEXT(binIn).toInt(&ok);

                if ((select == 1) && ok)
                {
                    txtOut << ""                                                                             << endl;
                    txtOut << "The autoban threshold is an integar value that determines how many"           << endl;
                    txtOut << "failed login attempts can be made to the " << ROOT_USER << " user before the" << endl;
                    txtOut << "offending ip address is blocked by the host."                                 << endl << endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 2) && ok)
                {
                    txtOut << ""                                                                                          << endl;
                    txtOut << "The autolock threshold is an integer value that determines how many"                       << endl;
                    txtOut << "failed login attempts can be made before the user account is locked"                       << endl;
                    txtOut << "by the host."                                                                              << endl << endl;
                    txtOut << "note: the " << ROOT_USER << " user never gets locked. instead, the offenders are blocked"  << endl;
                    txtOut << "      by ip address according to the autoban threshold."                                   << endl << endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 3) && ok)
                {
                    txtOut << ""                                                                       << endl;
                    txtOut << "Max sessions is an integar value that determines how many simultaneous" << endl;
                    txtOut << "clients the host will be allowed to run at once."                       << endl << endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 4) && ok)
                {
                    txtOut << ""                                                                     << endl;
                    txtOut << "Public registration basically allows un-logged in clients to run the" << endl;
                    txtOut << "new_user command. doing this allows un-registered users to become"    << endl;
                    txtOut << "registered users without the need to contact an admin."               << endl << endl;
                    txtOut << "[0] Disable"                                                          << endl;
                    txtOut << "[1] Enable"                                                           << endl << endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 5) && ok)
                {
                    txtOut << ""                                                                       << endl;
                    txtOut << "The initial host rank is the rank all new user accounts are registered" << endl;
                    txtOut << "with when created. the host rank itself is an integer value that"       << endl;
                    txtOut << "determine what commands each user can or cannot run."                   << endl << endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 6) && ok)
                {
                    txtOut << ""                                                              << endl;
                    txtOut << "This is the path to the command line email client executable"  << endl;
                    txtOut << "that the host can utilize to send emails to registered users." << endl << endl;
                    txtOut << "note: the host assumes the email application already has a"    << endl;
                    txtOut << "      configured sender email address/server."                 << endl << endl;
                    txtOut << "Enter a new path (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 7) && ok)
                {
                    txtOut << ""                                                                                       << endl;
                    txtOut << "This is the command line that will be used with the email client"                       << endl;
                    txtOut << "executable to send emails to registered users. it must contain the"                     << endl;
                    txtOut << "keywords " << SUBJECT_SUB << ", " << TARGET_EMAIL_SUB << " and " << MSG_SUB << " to be" << endl;
                    txtOut << "acceptable. the host will substitute these keywords for actual"                         << endl;
                    txtOut << "parameters when calling the email client."                                              << endl << endl;
                    txtOut << "Enter a new command line (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 8) && ok)
                {
                    txtOut << ""                                                              << endl;
                    txtOut << "This enables automated password resets via email so users can" << endl;
                    txtOut << "reset their account passwords without the need to contact an"  << endl;
                    txtOut << "admin. this basically tells the host if it is allowed to load" << endl;
                    txtOut << "the request_pw_reset and recover_account commands or not."     << endl << endl;
                    txtOut << "[0] Disable"                                                   << endl;
                    txtOut << "[1] Enable"                                                    << endl << endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 9) && ok)
                {
                    txtOut << ""                                                           << endl;
                    txtOut << "This enables automated email confirmations. this tells the" << endl;
                    txtOut << "host if it is allowed to load the confirm_email command."   << endl << endl;
                    txtOut << "[0] Disable"                                                << endl;
                    txtOut << "[1] Enable"                                                 << endl << endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 10) && ok)
                {
                    txtOut << ""                                                                       << endl;
                    txtOut << "This option tells the host if all sub-channels should be considered"    << endl;
                    txtOut << "active or not. otherwise, the active flag can be toggled on/off at the" << endl;
                    txtOut << "sub-channel level. active sub-channels send/receive PEER_INFO or"       << endl;
                    txtOut << "PEER_STAT frames with each other so all peers connected to the"         << endl;
                    txtOut << "sub-channel can be made aware of each other's public information."      << endl;
                    txtOut << "without the active flag, no such frames are transffered."               << endl << endl;
                    txtOut << "[0] Disable"                                                            << endl;
                    txtOut << "[1] Enable"                                                             << endl << endl;
                    txtOut << "Select an option (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 11) && ok)
                {
                    txtOut << ""                                                                     << endl;
                    txtOut << "This option sets the maximum amount of sub-channels each channel can" << endl;
                    txtOut << "have. the hard maximum is 256 and the minimum is 1."                  << endl << endl;
                    txtOut << "Enter a new value (leave blank to cancel): ";

                    level = 2;
                }
                else if ((select == 0) && ok)
                {
                    flags &= ~MORE_INPUT;
                }
                else
                {
                    txtOut << "" << endl << "Select an option: ";
                }

                mainTxt(txt);
            }
            else if (level == 2)
            {
                QString value = fromTEXT(binIn);

                if (value.isEmpty())
                {
                    mainTxt("\n");
                    returnToStart();
                }
                else
                {
                    if ((select == 1) || (select == 2) || (select == 3) || (select == 5))
                    {
                        bool    ok;
                        quint32 num = value.toUInt(&ok, 10);

                        if (!ok)
                        {
                            errTxt("err: Invalid 32bit unsigned integer. valid range: 1-4294967295.\n");
                            mainTxt("Enter a new value (leave blank to cancel): ");
                        }
                        else if (num == 0)
                        {
                            errTxt("err: This value cannot be 0, valid range: 1-4294967295.\n");
                            mainTxt("Enter a new value (leave blank to cancel): ");
                        }
                        else
                        {
                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);

                            if      (select == 1) db.addColumn(COLUMN_BAN_LIMIT, num);
                            else if (select == 2) db.addColumn(COLUMN_LOCK_LIMIT, num);
                            else if (select == 3) db.addColumn(COLUMN_MAXSESSIONS, num);
                            else                  db.addColumn(COLUMN_INITRANK, num);

                            db.exec();

                            if (select == 3)
                            {
                                async(ASYNC_MAXSES, PRIV_IPC, wrInt(num, BLKSIZE_HOST_LOAD * 8));
                            }

                            returnToStart();
                        }
                    }
                    else if ((select == 4) || (select == 8) || (select == 9) || (select == 10))
                    {
                        if (!isBool(value))
                        {
                            errTxt("err: Invalid boolean value. must be 0 (false) or 1 (true).\n");
                            mainTxt("Select an option (leave blank to cancel): ");
                        }
                        else
                        {
                            QString column;

                            if      (select == 4) column = COLUMN_PUB_USERS;
                            else if (select == 8) column = COLUMN_ENABLE_PW_RESET;
                            else if (select == 9) column = COLUMN_ENABLE_CONFIRM;
                            else                  column = COLUMN_ACTIVE_UPDATE;

                            Query db(this);

                            db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                            db.addColumn(column, static_cast<bool>(value.toUInt()));
                            db.exec();

                            returnToStart();
                        }
                    }
                    else if (select == 6)
                    {
                        if (!QFile::exists(expandEnvVariables(value)))
                        {
                            errTxt("err: The given file: '" + value + "' does not exists.\n");
                            mainTxt("Enter a new path (leave blank to cancel): ");
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
                    else if (select == 7)
                    {
                        if (!value.contains(SUBJECT_SUB, Qt::CaseInsensitive))
                        {
                            errTxt("err: The '" + QString(SUBJECT_SUB) + "' keyword is missing.\n");
                            mainTxt("Enter a new command line (leave blank to cancel): ");
                        }
                        else if (!value.contains(TARGET_EMAIL_SUB, Qt::CaseInsensitive))
                        {
                            errTxt("err: The '" + QString(TARGET_EMAIL_SUB) + "' keyword is missing.\n");
                            mainTxt("Enter a new command line (leave blank to cancel): ");
                        }
                        else if (!value.contains(MSG_SUB, Qt::CaseInsensitive))
                        {
                            errTxt("err: The '" + QString(MSG_SUB) + "' keyword is missing.\n");
                            mainTxt("Enter a new command line (leave blank to cancel): ");
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
                    else if (select == 11)
                    {
                        if (!isInt(value))
                        {
                            errTxt("err: '" + value + "' is not a valid integer.\n");
                            mainTxt("Enter a new value (leave blank to cancel): ");
                        }
                        else if ((value.toInt() < 1) || (value.toInt() > 256))
                        {
                            errTxt("err: A valid maximum sub-channels value ranges between 1-256.\n");
                            mainTxt("Enter a new value (leave blank to cancel): ");
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
