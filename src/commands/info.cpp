#include "info.h"

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

IPHist::IPHist(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_IPHIST, QStringList() << COLUMN_TIME << COLUMN_IPADDR << COLUMN_SESSION_ID << COLUMN_CLIENT_VER << COLUMN_LOGENTRY, true);
}

ListDBG::ListDBG(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_DMESG, QStringList() << COLUMN_TIME << COLUMN_LOGENTRY, true);
}

ListCommands::ListCommands(QObject *parent) : InternCommand(parent) {}
HostInfo::HostInfo(QObject *parent)         : InternCommand(parent) {}
MyInfo::MyInfo(QObject *parent)             : InternCommand(parent) {}
CmdInfo::CmdInfo(QObject *parent)           : InternCommand(parent) {}

QString ListCommands::cmdName() {return "ls_cmds";}
QString HostInfo::cmdName()     {return "host_info";}
QString IPHist::cmdName()       {return "ls_act_log";}
QString ListDBG::cmdName()      {return "ls_dbg";}
QString MyInfo::cmdName()       {return "my_info";}
QString CmdInfo::cmdName()      {return "cmd_info";}

void ListCommands::procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QString            find   = getParam("-find", parseArgs(data, 2));
        QList<quint16>     cmdIds = sharedObjs->cmdNames->keys();
        QList<QStringList> tableData;
        QStringList        separators;
        QList<int>         justLens;

        for (int i = 0; i < 3; ++i)
        {
            justLens.append(12);
            separators.append("-------");
        }

        tableData.append(QStringList() << "command_id" << "command_name" << "summary");
        tableData.append(separators);

        for (auto&& cmdId: cmdIds)
        {
            QStringList rowData;

            rowData.append(QString::number(cmdId));
            rowData.append(sharedObjs->cmdNames->value(cmdId));
            rowData.append(rwSharedObjs->commands->value(cmdId)->shortText());

            if (find.isEmpty() || rowData.contains(find, Qt::CaseInsensitive))
            {
                for (int k = 0; k < justLens.size(); ++k)
                {
                    if (justLens[k] < rowData[k].size()) justLens[k] = rowData[k].size();
                }

                tableData.append(rowData);
            }
        }

        mainTxt("\n");

        for (auto&& row : tableData)
        {
            for (int i = 0; i < row.size(); ++i)
            {
                mainTxt(row[i].leftJustified(justLens[i] + 2, ' '));
            }

            mainTxt("\n");
        }
    }
}

void HostInfo::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(binIn);
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        Query db(this);

        db.setType(Query::PULL, TABLE_SERV_SETTINGS);
        db.addColumn(COLUMN_IPADDR);
        db.addColumn(COLUMN_PORT);
        db.addColumn(COLUMN_MAXSESSIONS);
        db.exec();

        QString     txt;
        QTextStream txtOut(&txt);

        txtOut << "Application:    " << QCoreApplication::applicationName() << " v" << QCoreApplication::applicationVersion() << " " << QSysInfo::WordSize << "Bit" << endl;
        txtOut << "Import Rev:     " << IMPORT_REV << endl;
        txtOut << "Host Name:      " << QSysInfo::machineHostName() << endl;
        txtOut << "Host OS:        " << QSysInfo::prettyProductName() << endl;
        txtOut << "Load:           " << rdSessionLoad() << "/" << db.getData(COLUMN_MAXSESSIONS).toUInt() << endl;
        txtOut << "Listening Addr: " << db.getData(COLUMN_IPADDR).toString() << endl;
        txtOut << "Listening Port: " << db.getData(COLUMN_PORT).toUInt() << endl;

        mainTxt(txt);
    }
}

void MyInfo::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(binIn);

    if (dType == TEXT)
    {
        QString     txt;
        QTextStream txtOut(&txt);

        txtOut << "Session id:     " << sharedObjs->sessionId->toHex()            << endl;
        txtOut << "IP Address:     " << *sharedObjs->sessionAddr                  << endl;
        txtOut << "Logged-in:      " << boolStr(!sharedObjs->userName->isEmpty()) << endl;
        txtOut << "App Name:       " << *sharedObjs->appName                      << endl;

        if (!sharedObjs->userName->isEmpty())
        {
            Query db(this);

            db.setType(Query::PULL, TABLE_USERS);
            db.addColumn(COLUMN_EMAIL);
            db.addColumn(COLUMN_TIME);
            db.addColumn(COLUMN_EMAIL_VERIFIED);
            db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
            db.exec();

            txtOut << "User Name:      " << *sharedObjs->userName                               << endl;
            txtOut << "Group Name:     " << *sharedObjs->groupName                              << endl;
            txtOut << "Display Name:   " << *sharedObjs->displayName                            << endl;
            txtOut << "User ID:        " << sharedObjs->userId->toHex()                         << endl;
            txtOut << "Email:          " << db.getData(COLUMN_EMAIL).toString()                 << endl;
            txtOut << "Register Date:  " << db.getData(COLUMN_TIME).toString()                  << endl;
            txtOut << "Email Verified: " << boolStr(db.getData(COLUMN_EMAIL_VERIFIED).toBool()) << endl;
            txtOut << "Owner Override: " << boolStr(*sharedObjs->chOwnerOverride)               << endl;
            txtOut << "Host Rank:      " << *sharedObjs->hostRank                               << endl;
        }

        mainTxt(txt);
    }
}

void CmdInfo::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 2);
        QString     name  = getParam("-cmd_name", args);
        QString     cmdId = getParam("-cmd_id", args);

        if (name.isEmpty() && cmdId.isEmpty())
        {
            errTxt("err: Command name (-cmd_name) of command id (-cmd_id) parameter not found or is empty.\n");
        }
        else if (!name.isEmpty() && !validCommandName(name))
        {
            errTxt("err: Command name '" + name + "' is not valid.\n");
        }
        else if (!cmdId.isEmpty() && !isInt(cmdId))
        {
            errTxt("err: Command id '" + cmdId + "' is not a valid integer.\n");
        }
        else if (!name.isEmpty() && !sharedObjs->cmdNames->values().contains(name.toLower()))
        {
            errTxt("err: No such command name '" + name + "'\n");
        }
        else if (!cmdId.isEmpty() && !sharedObjs->cmdNames->contains(cmdId.toUShort()))
        {
            errTxt("err: No such command id '" + cmdId + "'\n");
        }
        else
        {   
            if (!name.isEmpty())
            {
                cmdId = QString::number(sharedObjs->cmdNames->key(name));
            }

            if (!cmdId.isEmpty())
            {
                name = sharedObjs->cmdNames->value(cmdId.toUShort());
            }

            QString     txt;
            QTextStream txtOut(&txt);

            ExternCommand *cmdObj = rwSharedObjs->commands->value(cmdId.toUShort());

            txtOut << "Command name: " << name                              << endl;
            txtOut << "Command id:   " << cmdId                             << endl;
            txtOut << "Gen file:     " << boolStr(cmdObj->handlesGenfile()) << endl << endl;

            txtOut << "IO:"               << endl << endl;
            txtOut << cmdObj->ioText()    << endl << endl;
            txtOut << "Summary:"          << endl << endl;
            txtOut << cmdObj->shortText() << endl << endl;
            txtOut << "Details:"          << endl << endl;

            mainTxt(txt);
            bigTxt(cmdObj->longText());
        }
    }
}
