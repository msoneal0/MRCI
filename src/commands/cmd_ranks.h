#ifndef CMD_RANKS_H
#define CMD_RANKS_H

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

#include "../common.h"
#include "../cmd_object.h"
#include "table_viewer.h"

bool commandHasRank(const QString &mod, const QString &cmdName);

class LsCmdRanks : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit LsCmdRanks(QObject *parent = nullptr);
};

//------------------------

class AssignCmdRank : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit AssignCmdRank(QObject *parent = nullptr);
};

//------------------------

class RemoveCmdRank : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RemoveCmdRank(QObject *parent = nullptr);
};

#endif // CMD_RANKS_H
