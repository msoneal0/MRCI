#ifndef AUTH_H
#define AUTH_H

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

class Auth : public InternCommand
{
    Q_OBJECT

private:

    QByteArray uId;
    QString    uName;
    QString    dName;
    bool       loginOk;
    bool       newPassword;
    bool       newUserName;

    void confirmAuth(const SharedObjs *sharedObjs);
    void addToThreshold(const SharedObjs *sharedObjs);

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit Auth(QObject *parent = nullptr);
};

//------------------------

class AuthLog : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit AuthLog(QObject *parent = nullptr);
};

#endif // AUTH_H
