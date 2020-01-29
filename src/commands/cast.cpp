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

Cast::Cast(QObject *parent)                         : CmdObject(parent) {}
OpenSubChannel::OpenSubChannel(QObject *parent)     : CmdObject(parent) {}
CloseSubChannel::CloseSubChannel(QObject *parent)   : CmdObject(parent) {}
LsOpenChannels::LsOpenChannels(QObject *parent)     : CmdObject(parent) {}
PingPeers::PingPeers(QObject *parent)               : CmdObject(parent) {}
AddRDOnlyFlag::AddRDOnlyFlag(QObject *parent)       : CmdObject(parent) {}
RemoveRDOnlyFlag::RemoveRDOnlyFlag(QObject *parent) : CmdObject(parent) {}

ListRDonlyFlags::ListRDonlyFlags(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_RDONLY_CAST, false);
    addJointColumn(TABLE_CHANNELS, COLUMN_CHANNEL_ID);
    addTableColumn(TABLE_CHANNELS, COLUMN_CHANNEL_NAME);
    addTableColumn(TABLE_RDONLY_CAST, COLUMN_CHANNEL_ID);
    addTableColumn(TABLE_RDONLY_CAST, COLUMN_SUB_CH_ID);
    addTableColumn(TABLE_RDONLY_CAST, COLUMN_ACCESS_LEVEL);
}

QString Cast::cmdName()             {return "cast";}
QString OpenSubChannel::cmdName()   {return "open_sub_ch";}
QString CloseSubChannel::cmdName()  {return "close_sub_ch";}
QString LsOpenChannels::cmdName()   {return "ls_open_chs";}
QString PingPeers::cmdName()        {return "ping_peers";}
QString AddRDOnlyFlag::cmdName()    {return "add_rdonly_flag";}
QString RemoveRDOnlyFlag::cmdName() {return "rm_rdonly_flag";}
QString ListRDonlyFlags::cmdName()  {return "ls_rdonly_flags";}

bool canOpenSubChannel(const QByteArray &uId, const char *override, quint64 chId, quint8 subId)
{
    auto uLevel = channelAccessLevel(uId, override, chId);
    auto sLevel = lowestAcessLevel(chId, subId);

    return uLevel <= sLevel;
}

int lowestAcessLevel(quint64 chId, quint8 subId)
{
    auto ret = 5000;

    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_LOWEST_LEVEL);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_SUB_CH_ID, subId);
    db.exec();

    if (db.rows())
    {
        ret = db.getData(COLUMN_LOWEST_LEVEL).toInt();
    }

    return ret;
}

void Cast::procIn(const QByteArray &binIn, quint8 dType)
{
    async(ASYNC_CAST, dType, binIn);
}

void OpenSubChannel::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 4);
        auto ch   = getParam("-ch_name", args);
        auto sub  = getParam("-sub_name", args);

        quint64 chId;
        quint8  subId;

        retCode = INVALID_PARAMS;

        if (ch.isEmpty())
        {
            errTxt("err: Channel name (-ch_name) argument not found or is empty.\n");
        }
        else if (sub.isEmpty())
        {
            errTxt("err: Sub-Channel name (-sub_name) argument not found or is empty.\n");
        }
        else if (!channelExists(ch, &chId))
        {
            errTxt("err: The requested channel does not exists.\n");
        }
        else if (!channelSubExists(chId, sub, &subId))
        {
            errTxt("err: The requested sub-channel does not exists.\n");
        }
        else if (!canOpenSubChannel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId, subId))
        {
            errTxt("err: Access denied.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            async(ASYNC_OPEN_SUBCH, PRIV_IPC, wrInt(chId, 64) + wrInt(subId, 8));
        }
    }
}

void CloseSubChannel::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 4);
        auto ch   = getParam("-ch_name", args);
        auto sub  = getParam("-sub_name", args);

        quint64 chId;
        quint8  subId;

        retCode = INVALID_PARAMS;

        if (ch.isEmpty())
        {
            errTxt("err: Channel name (-ch_name) argument not found or is empty.\n");
        }
        else if (sub.isEmpty())
        {
            errTxt("err: Sub-Channel name (-sub_name) argument not found or is empty.\n");
        }
        else if (!channelExists(ch, &chId))
        {
            errTxt("err: The requested channel does not exists.\n");
        }
        else if (!channelSubExists(chId, sub, &subId))
        {
            errTxt("err: The requested sub-channel does not exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            async(ASYNC_CLOSE_SUBCH, PRIV_IPC, wrInt(chId, 64) + wrInt(subId, 8));
        }
    }
}

void LsOpenChannels::procIn(const QByteArray &binIn, quint8 dType)
{
    Q_UNUSED(binIn)

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

        for (int i = 0; i < MAX_OPEN_SUB_CHANNELS; i += BLKSIZE_SUB_CHANNEL)
        {
            auto chId  = rd64BitFromBlock(openSubChs + i);
            auto subId = rd8BitFromBlock(openSubChs + (i + 8));

            if (chId)
            {
                QStringList columnData;

                db.setType(Query::INNER_JOIN_PULL, TABLE_SUB_CHANNELS);
                db.addTableColumn(TABLE_SUB_CHANNELS, COLUMN_SUB_CH_NAME);
                db.addTableColumn(TABLE_CHANNELS, COLUMN_CHANNEL_NAME);
                db.addJoinCondition(COLUMN_CHANNEL_ID, TABLE_CHANNELS);
                db.addCondition(COLUMN_CHANNEL_ID, chId);
                db.addCondition(COLUMN_SUB_CH_ID, subId);
                db.exec();

                auto chName  = db.getData(COLUMN_CHANNEL_NAME).toString();
                auto subName = db.getData(COLUMN_SUB_CH_NAME).toString();

                QString rdOnly;

                if (posOfBlock(openSubChs + i, openWritableSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL) == -1)
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
                mainTxt(row[i].leftJustified(justLens[i] + 4, ' '));
            }

            mainTxt("\n");
        }
    }
}

void PingPeers::procIn(const QByteArray &binIn, quint8 dType)
{
    Q_UNUSED(binIn)

    if (dType == TEXT)
    {
        if (rd8BitFromBlock(activeUpdate) == 0)
        {
            retCode = INVALID_PARAMS;

            errTxt("err: You don't currently have any active update sub-channels open. sending a ping request is pointless because peers won't be able to respond.\n");
        }
        else
        {
            async(ASYNC_PING_PEERS, PUB_IPC);
        }
    }
}

void AddRDOnlyFlag::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 6);
        auto chName = getParam("-ch_name", args);
        auto subId  = getParam("-sub_id", args);
        auto level  = getParam("-level", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

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
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (rdOnlyFlagExists(chName, static_cast<quint8>(subId.toInt()), level.toInt()))
        {
            errTxt("err: A read only flag for sub_id: " + QString::number(subId.toInt()) + " level: " + QString::number(level.toInt()) + " already exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            auto frame = wrInt(chId, 64) + wrInt(subId.toUInt(), 8) + wrInt(level.toUInt(), 8);

            Query db(this);

            db.setType(Query::PUSH, TABLE_RDONLY_CAST);
            db.addColumn(COLUMN_CHANNEL_ID, chId);
            db.addColumn(COLUMN_SUB_CH_ID, subId.toInt());
            db.addColumn(COLUMN_ACCESS_LEVEL, level.toInt());
            db.exec();

            async(ASYNC_ADD_RDONLY, PUB_IPC_WITH_FEEDBACK, frame);
        }
    }
}

void RemoveRDOnlyFlag::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 6);
        auto chName = getParam("-ch_name", args);
        auto subId  = getParam("-sub_id", args);
        auto level  = getParam("-level", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

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
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!rdOnlyFlagExists(chName, static_cast<quint8>(subId.toInt()), level.toInt()))
        {
            errTxt("err: A read only flag for sub_id: " + QString::number(subId.toInt()) + " level: " + QString::number(level.toInt()) + " does not exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            auto frame = wrInt(chId, 64) + wrInt(subId.toUInt(), 8) + wrInt(level.toUInt(), 8);

            Query db(this);

            db.setType(Query::DEL, TABLE_RDONLY_CAST);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_SUB_CH_ID, subId.toInt());
            db.addCondition(COLUMN_ACCESS_LEVEL, level.toInt());
            db.exec();

            async(ASYNC_RM_RDONLY, PUB_IPC_WITH_FEEDBACK, frame);
        }
    }
}

void ListRDonlyFlags::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            TableViewer::procIn(binIn, dType);
        }
        else
        {
            auto args   = parseArgs(binIn, 2);
            auto chName = getParam("-ch_name", args);

            quint64 chId;

            retCode = INVALID_PARAMS;

            if (chName.isEmpty())
            {
                errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
            }
            else if (!validChName(chName))
            {
                errTxt("err: Invalid channel name.\n");
            }
            else if (!channelExists(chName, &chId))
            {
                errTxt("err: Channel name '" + chName + "' does not exists.\n");
            }
            else
            {
                retCode = NO_ERRORS;
                
                if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > REGULAR)
                {
                    TableViewer::procIn(toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName + " -" + QString(COLUMN_LOWEST_LEVEL) + " " + QString::number(PUBLIC)), dType);
                }
                else
                {
                    TableViewer::procIn(toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName), dType);
                }
            }
        }
    }
}
