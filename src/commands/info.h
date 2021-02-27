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
#include "../cmd_object.h"
#include "../shell.h"
#include "table_viewer.h"
#include "fs.h"
#include "acct_recovery.h"

class ListCommands : public CmdObject
{
    Q_OBJECT

private:

    QStringList list;

    QString shortText(const QString &cmdName);
    QString ioText(const QString &cmdName);
    QString longText(const QString &cmdName);

private slots:

    void onIPCConnected();

public:

    explicit ListCommands(const QStringList &cmdList, QObject *parent = nullptr);
};

//--------------------------------------

class HostInfo : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

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

class MyInfo : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit MyInfo(QObject *parent = nullptr);
};

#endif // INFO_H
