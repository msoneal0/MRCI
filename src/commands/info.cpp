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

ListCommands::ListCommands(const QStringList &cmdList, QObject *parent) : CmdObject(parent)
{
    list = cmdList;
}

HostInfo::HostInfo(QObject *parent) : CmdObject(parent) {}
MyInfo::MyInfo(QObject *parent)     : CmdObject(parent) {}

QString HostInfo::cmdName() {return "host_info";}
QString IPHist::cmdName()   {return "ls_act_log";}
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
        auto genType = QByteArray(1, 0x00);

        if (cmdName == DownloadFile::cmdName())
        {
            genType = QByteArray(1, GEN_DOWNLOAD);
        }
        else if (cmdName == UploadFile::cmdName())
        {
            genType = QByteArray(1, GEN_UPLOAD);
        }

        QByteArray frame;

        frame.append(QByteArray(2, 0x00));
        frame.append(genType);
        frame.append(toFixedTEXT(cmdName, 64));
        frame.append(toFixedTEXT(libName(), 64));
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
        QString     txt;
        QTextStream txtOut(&txt);

        hostSharedMem->lock();

        auto confObj  = confObject();
        auto sesCount = rd32BitFromBlock(hostLoad);
        auto maxSes   = confObj[CONF_MAX_SESSIONS].toInt();
        auto addr     = confObj[CONF_LISTEN_ADDR].toString();
        auto port     = confObj[CONF_LISTEN_PORT].toInt();

        hostSharedMem->unlock();

        txtOut << "Application:    " << libName() << Qt::endl;
        txtOut << "Qt Base:        " << QT_VERSION_STR << Qt::endl;
        txtOut << "Host Name:      " << QSysInfo::machineHostName() << Qt::endl;
        txtOut << "Host OS:        " << QSysInfo::prettyProductName() << Qt::endl;
        txtOut << "Load:           " << sesCount << "/" << maxSes << Qt::endl;
        txtOut << "Listening Addr: " << addr << Qt::endl;
        txtOut << "Listening Port: " << port << Qt::endl;

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

        auto sesId = rdFromBlock(sessionId, BLKSIZE_SESSION_ID).toHex();
        auto ip    = rdStringFromBlock(clientIp, BLKSIZE_CLIENT_IP);
        auto app   = rdStringFromBlock(appName, BLKSIZE_APP_NAME);

        txtOut << "Session id:     " << sesId << Qt::endl;
        txtOut << "IP Address:     " << ip    << Qt::endl;
        txtOut << "App Name:       " << app   << Qt::endl;

        if (!isEmptyBlock(userId, BLKSIZE_USER_ID))
        {
            auto uId = rdFromBlock(userId, BLKSIZE_USER_ID);

            Query db(this);

            db.setType(Query::PULL, TABLE_USERS);
            db.addColumn(COLUMN_EMAIL);
            db.addColumn(COLUMN_TIME);
            db.addColumn(COLUMN_EMAIL_VERIFIED);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            txtOut << "User Name:      " << rdStringFromBlock(userName, BLKSIZE_USER_NAME)      << Qt::endl;
            txtOut << "Display Name:   " << rdStringFromBlock(displayName, BLKSIZE_DISP_NAME)   << Qt::endl;
            txtOut << "User id:        " << uId.toHex()                                         << Qt::endl;
            txtOut << "Email:          " << db.getData(COLUMN_EMAIL).toString()                 << Qt::endl;
            txtOut << "Register Date:  " << db.getData(COLUMN_TIME).toString()                  << Qt::endl;
            txtOut << "Email Verified: " << boolStr(db.getData(COLUMN_EMAIL_VERIFIED).toBool()) << Qt::endl;
            txtOut << "Owner Override: " << boolStr(rd8BitFromBlock(chOwnerOverride))           << Qt::endl;
            txtOut << "Host Rank:      " << rd32BitFromBlock(hostRank)                          << Qt::endl;
        }

        mainTxt(txt);
    }
}
