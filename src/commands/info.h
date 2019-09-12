#ifndef INFO_H
#define INFO_H

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
#include "table_viewer.h"

class ListCommands : public InternCommand
{
    Q_OBJECT

private:

    bool strInRowTxt(const QString &str, const QStringList &rowTxt);

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType);

    explicit ListCommands(QObject *parent = nullptr);
};

//--------------------------------------

class HostInfo : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit HostInfo(QObject *parent = nullptr);
};

//------------------------------------

class IPHist : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit IPHist(QObject *parent = nullptr);
};

//------------------------------------

class MyInfo : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit MyInfo(QObject *parent = nullptr);
};

//-----------------------------------

class ListDBG : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit ListDBG(QObject *parent = nullptr);
};

//----------------------------------

class CmdInfo : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit CmdInfo(QObject *parent = nullptr);
};

#endif // INFO_H
