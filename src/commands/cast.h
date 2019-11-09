#ifndef CAST_CMDS_H
#define CAST_CMDS_H

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

bool canOpenSubChannel(const QByteArray &uId, const char *override, quint64 chId, quint8 subId);
int  lowestAcessLevel(quint64 chId, quint8 subId);

class Cast : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit Cast(QObject *parent = nullptr);
};

//----------------------------------

class OpenSubChannel : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit OpenSubChannel(QObject *parent = nullptr);
};

//-----------------------------------

class CloseSubChannel : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit CloseSubChannel(QObject *parent = nullptr);
};

//----------------------------------

class LsOpenChannels : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit LsOpenChannels(QObject *parent = nullptr);
};

//----------------------------------

class PingPeers : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit PingPeers(QObject *parent = nullptr);
};

//---------------------------------

class AddRDOnlyFlag : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit AddRDOnlyFlag(QObject *parent = nullptr);
};

//-------------------------------

class RemoveRDOnlyFlag : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RemoveRDOnlyFlag(QObject *parent = nullptr);
};

//------------------------------

class ListRDonlyFlags : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ListRDonlyFlags(QObject *parent = nullptr);
};

#endif // CAST_CMDS_H
