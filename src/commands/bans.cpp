#include "bans.h"

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

ListBans::ListBans(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_IPBANS, QStringList() << COLUMN_TIME << COLUMN_IPADDR, true);
}

BanIP::BanIP(QObject *parent)     : InternCommand(parent) {}
UnBanIP::UnBanIP(QObject *parent) : InternCommand(parent) {}

QString ListBans::cmdName() {return "ls_bans";}
QString BanIP::cmdName()    {return "add_ban";}
QString UnBanIP::cmdName()  {return "rm_ban";}

void BanIP::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 2);
        QString     ip   = getParam("-ip", args);

        if (ip.isEmpty())
        {
            errTxt("err: The ip address argument (-ip) was not found or is empty.\n");
        }
        else if (!QHostAddress().setAddress(ip))
        {
            errTxt("err: '" + ip + "' is not a valid ip address.\n");
        }
        else
        {
            QHostAddress addr(ip);

            Query db(this);

            db.setType(Query::PUSH, TABLE_IPBANS);
            db.addColumn(COLUMN_IPADDR, addr.toString());
            db.exec();
        }
    }
}

void UnBanIP::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 2);
        QString     ip   = getParam("-ip", args);

        if (ip.isEmpty())
        {
            errTxt("err: The ip address argument (-ip) was not found or is empty.\n");
        }
        else if (!QHostAddress().setAddress(ip))
        {
            errTxt("err: '" + ip + "' is not a valid ip address.\n");
        }
        else
        {
            QHostAddress addr(ip);

            Query db;

            db.setType(Query::DEL, TABLE_IPBANS);
            db.addCondition(COLUMN_IPADDR, addr.toString());
            db.exec();
        }
    }
}
