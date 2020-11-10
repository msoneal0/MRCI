#ifndef ACCT_RECOVERY_H
#define ACCT_RECOVERY_H

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

bool expired(const QByteArray &uId);
void delRecoverPw(const QByteArray &uId);

class RecoverAcct : public CmdObject
{
    Q_OBJECT

private:

    QByteArray uId;
    bool       inputOk;

    void addToThreshold();

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RecoverAcct(QObject *parent = nullptr);
};

//---------------

class ResetPwRequest : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, uchar dType);

    explicit ResetPwRequest(QObject *parent = nullptr);
};

//----------------

class VerifyEmail : public CmdObject
{
    Q_OBJECT

private:

    QString    code;
    QByteArray uId;

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit VerifyEmail(QObject *parent = nullptr);
};

//----------------

class IsEmailVerified : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit IsEmailVerified(QObject *parent = nullptr);
};

#endif // ACCT_RECOVERY_H
