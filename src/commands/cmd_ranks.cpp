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

bool commandHasRank(const QString &mod, const QString &cmdName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CMD_RANKS);
    db.addColumn(COLUMN_COMMAND);
    db.addCondition(COLUMN_MOD_MAIN, mod);
    db.addCondition(COLUMN_COMMAND, cmdName);
    db.exec();

    return db.rows();
}

LsCmdRanks::LsCmdRanks(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CMD_RANKS, false);

    addTableColumn(TABLE_CMD_RANKS, COLUMN_MOD_MAIN);
    addTableColumn(TABLE_CMD_RANKS, COLUMN_COMMAND);
    addTableColumn(TABLE_CMD_RANKS, COLUMN_HOST_RANK);
}

AssignCmdRank::AssignCmdRank(QObject *parent) : CmdObject(parent) {}
RemoveCmdRank::RemoveCmdRank(QObject *parent) : CmdObject(parent) {}

QString LsCmdRanks::cmdName()    {return "ls_ranked_cmds";}
QString AssignCmdRank::cmdName() {return "add_ranked_cmd";}
QString RemoveCmdRank::cmdName() {return "rm_ranked_cmd";}

void AssignCmdRank::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 6);
        auto cmdName = getParam("-command", args);
        auto mod     = getParam("-mod", args);
        auto rank    = getParam("-rank", args);

        retCode = INVALID_PARAMS;

        if (cmdName.isEmpty())
        {
            errTxt("err: The command name (-command) argument was not found or is empty.\n");
        }
        else if (rank.isEmpty())
        {
            errTxt("err: The rank (-rank) argument was not found or is empty.\n");
        }
        else if (mod.isEmpty())
        {
            errTxt("err: The module path (-mod) argument was not found or is empty.\n");
        }
        else if (!isInt(rank))
        {
            errTxt("err: The given rank is not a valid 32bit unsigned integer.\n");
        }
        else if (!validCommandName(cmdName))
        {
            errTxt("err: Invalid command name. it must be 1-64 chars long and contain no spaces.\n");
        }
        else if (!validModPath(mod))
        {
            errTxt("err: Invalid module path. it must be less than 512 chars long and it cannot contain any of the following chars '|*:\"?<>'\n");
        }
        else if (commandHasRank(mod, cmdName))
        {
            errTxt("err: The given mod - command name combo already has an assigned rank.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::PUSH, TABLE_CMD_RANKS);
            db.addColumn(COLUMN_COMMAND, cmdName);
            db.addColumn(COLUMN_MOD_MAIN, mod);
            db.addColumn(COLUMN_HOST_RANK, rank.toUInt());
            db.exec();

            async(ASYNC_CMD_RANKS_CHANGED);
        }
    }
}

void RemoveCmdRank::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 4);
        auto cmdName = getParam("-command", args);
        auto mod     = getParam("-mod", args);

        retCode = INVALID_PARAMS;

        if (cmdName.isEmpty())
        {
            errTxt("err: The command name (-command) argument was not found or is empty.\n");
        }
        else if (mod.isEmpty())
        {
            errTxt("err: The module path (-mod) argument was not found or is empty.\n");
        }
        else if (!validCommandName(cmdName))
        {
            errTxt("err: Invalid command name.\n");
        }
        else if (!validModPath(mod))
        {
            errTxt("err: Invalid module path.\n");
        }
        else if (!commandHasRank(mod, cmdName))
        {
            errTxt("err: The given mod - command name combo does not have an assigned rank.\n");
        }
        else
        {
            retCode = NO_ERRORS;
            
            Query db(this);

            db.setType(Query::DEL, TABLE_CMD_RANKS);
            db.addCondition(COLUMN_COMMAND, cmdName);
            db.addCondition(COLUMN_MOD_MAIN, mod);
            db.exec();

            async(ASYNC_CMD_RANKS_CHANGED);
        }
    }
}
