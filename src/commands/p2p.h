#ifndef P2P_H
#define P2P_H

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

class ToPeer : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ToPeer(QObject *parent = nullptr);
};

//----------------------------------

class P2PRequest : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit P2PRequest(QObject *parent = nullptr);
};

//-----------------------------------

class P2POpen : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit P2POpen(QObject *parent = nullptr);
};

//----------------------------------

class P2PClose : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit P2PClose(QObject *parent = nullptr);
};

//----------------------------------

class LsP2P : public CmdObject
{
    Q_OBJECT

private:

    QList<QByteArray> lsBlocks(const char* blocks, int maxBlocks, int sizeOfBlock);

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit LsP2P(QObject *parent = nullptr);
};

#endif // P2P_H
