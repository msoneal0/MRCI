#include "cast.h"

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

Cast::Cast(QObject *parent)                         : InternCommand(parent) {}
OpenSubChannel::OpenSubChannel(QObject *parent)     : InternCommand(parent) {}
CloseSubChannel::CloseSubChannel(QObject *parent)   : InternCommand(parent) {}
LsOpenChannels::LsOpenChannels(QObject *parent)     : InternCommand(parent) {}
PingPeers::PingPeers(QObject *parent)               : InternCommand(parent) {}
AddRDOnlyFlag::AddRDOnlyFlag(QObject *parent)       : InternCommand(parent) {}
RemoveRDOnlyFlag::RemoveRDOnlyFlag(QObject *parent) : InternCommand(parent) {}

ListRDonlyFlags::ListRDonlyFlags(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_RDONLY_CAST, QStringList() << COLUMN_SUB_CH_ID << COLUMN_ACCESS_LEVEL << COLUMN_CHANNEL_NAME, false);
}

QString Cast::cmdName()             {return "cast";}
QString OpenSubChannel::cmdName()   {return "open_sub_ch";}
QString CloseSubChannel::cmdName()  {return "close_sub_ch";}
QString LsOpenChannels::cmdName()   {return "ls_open_chs";}
QString PingPeers::cmdName()        {return "ping_peers";}
QString AddRDOnlyFlag::cmdName()    {return "add_rdonly_flag";}
QString RemoveRDOnlyFlag::cmdName() {return "rm_rdonly_flag";}
QString ListRDonlyFlags::cmdName()  {return "ls_rdonly_flags";}

void Cast::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    emit castToPeers(binIn, dType);
}

void OpenSubChannel::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 4);
        QString     ch   = getParam("-ch_name", args);
        QString     sub  = getParam("-sub_name", args);

        if (ch.isEmpty())
        {
            errTxt("err: Channel name (-ch_name) argument not found or is empty.\n");
        }
        else if (sub.isEmpty())
        {
            errTxt("err: Sub-Channel name (-sub_name) argument not found or is empty.\n");
        }
        else
        {
            emit openChByName(ch, sub);
        }
    }
}

void CloseSubChannel::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 4);
        QString     ch   = getParam("-ch_name", args);
        QString     sub  = getParam("-sub_name", args);

        if (ch.isEmpty())
        {
            errTxt("err: Channel name (-ch_name) argument not found or is empty.\n");
        }
        else if (sub.isEmpty())
        {
            errTxt("err: Sub-Channel name (-sub_name) argument not found or is empty.\n");
        }
        else
        {
            emit closeChByName(ch, sub);
        }
    }
}

void LsOpenChannels::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(binIn);

    if (dType == TEXT)
    {
        Query              db;
        QList<QStringList> tableData;
        QStringList        separators;
        QList<int>         justLens;

        for (int i = 0; i < 5; ++i)
        {
            justLens.append(14);
            separators.append("-------");
        }

        tableData.append(QStringList() << COLUMN_CHANNEL_NAME << COLUMN_SUB_CH_NAME << COLUMN_CHANNEL_ID << COLUMN_SUB_CH_ID << "read_only");
        tableData.append(separators);

        for (int i = 0; i < sharedObjs->chIds->size(); i += 9)
        {
            quint64 chId  = rdInt(QByteArray::fromRawData(sharedObjs->chIds->data() + i, 8));
            quint64 subId = rdInt(QByteArray::fromRawData(sharedObjs->chIds->data() + (i + 8), 1));

            if (chId)
            {
                QStringList columnData;

                db.setType(Query::PULL, TABLE_SUB_CHANNELS);
                db.addColumn(COLUMN_SUB_CH_NAME);
                db.addColumn(COLUMN_CHANNEL_NAME);
                db.addCondition(COLUMN_CHANNEL_ID, chId);
                db.addCondition(COLUMN_SUB_CH_ID, subId);
                db.exec();

                QByteArray subCh   = QByteArray::fromRawData(sharedObjs->chIds->data() + i, 9);
                QString    chName  = db.getData(COLUMN_CHANNEL_NAME).toString();
                QString    subName = db.getData(COLUMN_SUB_CH_NAME).toString();
                QString    rdOnly;

                if (chPos(subCh, *sharedObjs->wrAbleChIds) != -1)
                {
                    rdOnly = "1";
                }
                else
                {
                    rdOnly = "0";
                }

                columnData.append(chName);
                columnData.append(subName);
                columnData.append(QString::number(chId));
                columnData.append(QString::number(subId));
                columnData.append(rdOnly);

                for (int k = 0; k < justLens.size(); ++k)
                {
                    if (justLens[k] < columnData[k].size()) justLens[k] = columnData[k].size();
                }

                tableData.append(columnData);
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

void PingPeers::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(binIn);

    if (dType == TEXT)
    {
        if (!(*sharedObjs->activeUpdate))
        {
            errTxt("err: You don't currently have any active update sub-channels open. sending a ping request is pointless because peers won't be able to respond.\n");
        }
        else
        {
            QByteArray castHeader = *sharedObjs->chIds + wrInt(PING_PEERS, 8);
            QByteArray data       = toPEER_INFO(sharedObjs);

            emit backendDataOut(ASYNC_LIMITED_CAST, castHeader + data, PUB_IPC);
        }
    }
}

void AddRDOnlyFlag::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 6);
        QString     chName = getParam("-ch_name", args);
        QString     subId  = getParam("-sub_id", args);
        QString     level  = getParam("-level", args);

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (subId.isEmpty())
        {
            errTxt("err: The sub-channel id (-sub_id) was not found or is empty.\n");
        }
        else if (level.isEmpty())
        {
            errTxt("err: The privilage level (-level) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validSubId(subId))
        {
            errTxt("err: Invalid sub-channel id. valid range (0-255).\n");
        }
        else if (!validLevel(level, true))
        {
            errTxt("err: Invalid privilage level. valid range (1-5).\n");
        }
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(sharedObjs, chName) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (rdOnlyFlagExists(chName, static_cast<uchar>(subId.toInt()), level.toInt()))
        {
            errTxt("err: A read only flag for sub-id: " + QString::number(subId.toInt()) + " level: " + QString::number(level.toInt()) + " already exists.\n");
        }
        else
        {   
            Query db(this);

            db.setType(Query::PUSH, TABLE_RDONLY_CAST);
            db.addColumn(COLUMN_CHANNEL_NAME, chName);
            db.addColumn(COLUMN_SUB_CH_ID, subId.toInt());
            db.addColumn(COLUMN_ACCESS_LEVEL, level.toInt());
            db.exec();

            emit backendDataOut(ASYNC_ADD_RDONLY, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RemoveRDOnlyFlag::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 6);
        QString     chName = getParam("-ch_name", args);
        QString     subId  = getParam("-sub_id", args);
        QString     level  = getParam("-level", args);

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (subId.isEmpty())
        {
            errTxt("err: The sub-channel id (-sub_id) was not found or is empty.\n");
        }
        else if (level.isEmpty())
        {
            errTxt("err: The privilage level (-level) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validSubId(subId))
        {
            errTxt("err: Invalid sub-channel id. valid range (0-255).\n");
        }
        else if (!validLevel(level, true))
        {
            errTxt("err: Invalid privilage level. valid range (1-5).\n");
        }
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(sharedObjs, chName) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!rdOnlyFlagExists(chName, static_cast<uchar>(subId.toInt()), level.toInt()))
        {
            errTxt("err: A read only flag for sub-id: " + QString::number(subId.toInt()) + " level: " + QString::number(level.toInt()) + " does not exists.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::DEL, TABLE_RDONLY_CAST);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.addCondition(COLUMN_SUB_CH_ID, subId.toInt());
            db.addCondition(COLUMN_ACCESS_LEVEL, level.toInt());
            db.exec();

            emit backendDataOut(ASYNC_RM_RDONLY, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void ListRDonlyFlags::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        if (moreInputEnabled())
        {
            TableViewer::procBin(sharedObjs, binIn, dType);
        }
        else
        {
            QStringList args   = parseArgs(binIn, 2);
            QString     chName = getParam("-ch_name", args);

            if (chName.isEmpty())
            {
                errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
            }
            else if (!validChName(chName))
            {
                errTxt("err: Invalid channel name.\n");
            }
            else if (!channelExists(chName))
            {
                errTxt("err: Channel name '" + chName + "' does not exists.\n");
            }
            else
            {
                if (channelAccessLevel(sharedObjs, chName) > REGULAR)
                {
                    TableViewer::procBin(sharedObjs, toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName + " -" + QString(COLUMN_LOWEST_LEVEL) + " " + QString::number(PUBLIC)), dType);
                }
                else
                {
                    TableViewer::procBin(sharedObjs, toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName), dType);
                }
            }
        }
    }
}
