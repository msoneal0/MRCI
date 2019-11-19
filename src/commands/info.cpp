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
    setParams(TABLE_IPHIST, true);
    addTableColumn(TABLE_IPHIST, COLUMN_TIME);
    addTableColumn(TABLE_IPHIST, COLUMN_IPADDR);
    addTableColumn(TABLE_IPHIST, COLUMN_APP_NAME);
    addTableColumn(TABLE_IPHIST, COLUMN_SESSION_ID);
    addTableColumn(TABLE_IPHIST, COLUMN_LOGENTRY);
}

ListDBG::ListDBG(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_DMESG, true);
    addTableColumn(TABLE_DMESG, COLUMN_TIME);
    addTableColumn(TABLE_DMESG, COLUMN_LOGENTRY);
}

ListCommands::ListCommands(const QStringList &cmdList, const QStringList &gen, QObject *parent) : CmdObject(parent)
{
    list        = cmdList;
    genfileList = gen;
}

HostInfo::HostInfo(QObject *parent) : CmdObject(parent) {}
MyInfo::MyInfo(QObject *parent)     : CmdObject(parent) {}

QString HostInfo::cmdName() {return "host_info";}
QString IPHist::cmdName()   {return "ls_act_log";}
QString ListDBG::cmdName()  {return "ls_dbg";}
QString MyInfo::cmdName()   {return "my_info";}

QString ListCommands::shortText(const QString &cmdName)
{
    return parseMd(cmdName, 1);
}

QString ListCommands::ioText(const QString &cmdName)
{
    return parseMd(cmdName, 2);
}

QString ListCommands::longText(const QString &cmdName)
{
    return parseMd(cmdName, 3);
}

void ListCommands::onIPCConnected()
{
    for (auto&& cmdName : list)
    {
        QByteArray frame;
        QByteArray boolByte = QByteArray(1, 0x00);

        if (genfileList.contains(cmdName))
        {
            boolByte = QByteArray(1, 0x01);
        }

        frame.append(QByteArray(2, 0x00));
        frame.append(boolByte);
        frame.append(fixedToTEXT(cmdName, BLKSIZE_CMD_NAME));
        frame.append(fixedToTEXT(libName(), BLKSIZE_LIB_NAME));
        frame.append(nullTermTEXT(shortText(cmdName)));
        frame.append(nullTermTEXT(ioText(cmdName)));
        frame.append(nullTermTEXT(longText(cmdName)));

        emit procOut(frame, NEW_CMD);
    }
}

void HostInfo::procIn(const QByteArray &binIn, quint8 dType)
{
    Q_UNUSED(binIn)

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

        hostSharedMem->lock();

        quint32 sesCount = rd32BitFromBlock(hostLoad);
        quint32 maxSes   = db.getData(COLUMN_MAXSESSIONS).toUInt();

        hostSharedMem->unlock();

        txtOut << "Application:    " << libName() << endl;
        txtOut << "Qt Base:        " << QT_VERSION_STR << endl;
        txtOut << "Host Name:      " << QSysInfo::machineHostName() << endl;
        txtOut << "Host OS:        " << QSysInfo::prettyProductName() << endl;
        txtOut << "Load:           " << sesCount << "/" << maxSes << endl;
        txtOut << "Listening Addr: " << db.getData(COLUMN_IPADDR).toString() << endl;
        txtOut << "Listening Port: " << db.getData(COLUMN_PORT).toUInt() << endl;

        mainTxt(txt);
    }
}

void MyInfo::procIn(const QByteArray &binIn, quint8 dType)
{
    Q_UNUSED(binIn)

    if (dType == TEXT)
    {
        QString     txt;
        QTextStream txtOut(&txt);

        QString sesId = rdFromBlock(sessionId, BLKSIZE_SESSION_ID).toHex();
        QString ip    = rdStringFromBlock(clientIp, BLKSIZE_CLIENT_IP);
        QString app   = rdStringFromBlock(appName, BLKSIZE_APP_NAME);

        txtOut << "Session id:     " << sesId << endl;
        txtOut << "IP Address:     " << ip    << endl;
        txtOut << "App Name:       " << app   << endl;

        if (!isEmptyBlock(userId, BLKSIZE_USER_ID))
        {
            QByteArray uId = rdFromBlock(userId, BLKSIZE_USER_ID);

            Query db(this);

            db.setType(Query::PULL, TABLE_USERS);
            db.addColumn(COLUMN_EMAIL);
            db.addColumn(COLUMN_TIME);
            db.addColumn(COLUMN_EMAIL_VERIFIED);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            txtOut << "User Name:      " << rdStringFromBlock(userName, BLKSIZE_USER_NAME)      << endl;
            txtOut << "Display Name:   " << rdStringFromBlock(displayName, BLKSIZE_DISP_NAME)   << endl;
            txtOut << "User id:        " << uId.toHex()                                         << endl;
            txtOut << "Email:          " << db.getData(COLUMN_EMAIL).toString()                 << endl;
            txtOut << "Register Date:  " << db.getData(COLUMN_TIME).toString()                  << endl;
            txtOut << "Email Verified: " << boolStr(db.getData(COLUMN_EMAIL_VERIFIED).toBool()) << endl;
            txtOut << "Owner Override: " << boolStr(rd8BitFromBlock(chOwnerOverride))           << endl;
            txtOut << "Host Rank:      " << rd32BitFromBlock(hostRank)                          << endl;
        }

        mainTxt(txt);
    }
}
