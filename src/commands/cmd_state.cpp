#include "cmd_state.h"

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

Term::Term(QObject *parent)     : InternCommand(parent) {}
Pause::Pause(QObject *parent)   : InternCommand(parent) {}
Resume::Resume(QObject *parent) : InternCommand(parent) {}

QString Term::cmdName()   {return "term";}
QString Pause::cmdName()  {return "pause";}
QString Resume::cmdName() {return "resume";}

void Term::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == CMD_ID)
    {
        if (binIn.isEmpty())
        {
            emit termAllCommands();
        }
        else
        {
            auto cmdId = static_cast<quint16>(rdInt(binIn));

            if (!sharedObjs->cmdNames->contains(cmdId))
            {
                errTxt("err: No such command id: '" + QString::number(cmdId) + "'\n");
            }
            else
            {
                emit termCommandId(cmdId);
            }
        }
    }
}

void Pause::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == CMD_ID)
    {
        if (binIn.isEmpty())
        {
            *rwSharedObjs->pausedCmds = *rwSharedObjs->activeLoopCmds;
        }
        else
        {
            auto cmdId = static_cast<quint16>(rdInt(binIn));

            if (!sharedObjs->cmdNames->contains(cmdId))
            {
                errTxt("err: No such command id: '" + QString::number(cmdId) + "'\n");
            }
            else if (!sharedObjs->activeLoopCmds->contains(cmdId))
            {
                errTxt("err: The command is not currently in a loop state.\n");
            }
            else
            {
                uniqueAdd(cmdId, *rwSharedObjs->pausedCmds);
            }
        }
    }
}

void Resume::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == CMD_ID)
    {
        if (binIn.isEmpty())
        {
            rwSharedObjs->pausedCmds->clear();
        }
        else
        {
            auto cmdId = static_cast<quint16>(rdInt(binIn));

            if (!sharedObjs->cmdNames->contains(cmdId))
            {
                errTxt("err: No such command id: '" + QString::number(cmdId) + "'\n");
            }
            else if (!sharedObjs->pausedCmds->contains(cmdId))
            {
                errTxt("err: The command is not currently in a paused state.\n");
            }
            else
            {
                rwSharedObjs->pausedCmds->removeAll(cmdId);
            }
        }
    }
}
