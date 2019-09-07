#ifndef ADMIN_CMDS_H
#define ADMIN_CMDS_H

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

class CloseHost : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit CloseHost(QObject *parent = nullptr);
};

//--------------------------------------

class RestartHost : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit RestartHost(QObject *parent = nullptr);
};

//--------------------------------------

class ServSettings : public InternCommand
{
    Q_OBJECT

private:

    int level;
    int select;

    void printOptions();
    void printSettings();
    void returnToStart();

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ServSettings(QObject *parent = nullptr);
};

#endif // ADMIN_CMDS_H
