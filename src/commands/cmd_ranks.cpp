#include "cmd_ranks.h"

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

LsCmdRanks::LsCmdRanks(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CMD_RANKS, QStringList() << COLUMN_COMMAND << COLUMN_HOST_RANK, false);
}

AssignCmdRank::AssignCmdRank(QObject *parent) : InternCommand(parent) {}
RemoveCmdRank::RemoveCmdRank(QObject *parent) : InternCommand(parent) {}

QString LsCmdRanks::cmdName()    {return "ls_ranked_cmds";}
QString AssignCmdRank::cmdName() {return "add_ranked_cmd";}
QString RemoveCmdRank::cmdName() {return "rm_ranked_cmd";}

void AssignCmdRank::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 4);
        QString     cmdName = getParam("-command", args);
        QString     rank    = getParam("-rank", args);

        if (cmdName.isEmpty())
        {
            errTxt("err: The command name (-command) argument was not found or is empty.\n");
        }
        else if (rank.isEmpty())
        {
            errTxt("err: The rank (-rank) argument was not found or is empty.\n");
        }
        else if (!isInt(rank))
        {
            errTxt("err: The given rank is not a valid unsigned integer.\n");
        }
        else if (!validCommandName(cmdName))
        {
            errTxt("err: Invalid command name. it must be 1-64 chars long and can only contain letters, numbers, '_' or '?'.\n");
        }
        else if (commandHasRank(cmdName))
        {
            errTxt("err: The given command name already has an assigned rank.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::PUSH, TABLE_CMD_RANKS);
            db.addColumn(COLUMN_COMMAND, cmdName);
            db.addColumn(COLUMN_HOST_RANK, rank.toUInt());
            db.exec();

            emit backendDataOut(ASYNC_CMD_RANKS_CHANGED, QByteArray(), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RemoveCmdRank::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 2);
        QString     cmdName = getParam("-command", args);

        if (cmdName.isEmpty())
        {
            errTxt("err: The command name (-command) argument was not found or is empty.\n");
        }
        else if (!validCommandName(cmdName))
        {
            errTxt("err: Invalid command name.\n");
        }
        else if (!commandHasRank(cmdName))
        {
            errTxt("err: The given command name does not have an assigned rank.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::DEL, TABLE_CMD_RANKS);
            db.addCondition(COLUMN_COMMAND, cmdName);
            db.exec();

            emit backendDataOut(ASYNC_CMD_RANKS_CHANGED, QByteArray(), PUB_IPC_WITH_FEEDBACK);
        }
    }
}
