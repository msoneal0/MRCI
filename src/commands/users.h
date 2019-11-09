#ifndef USERS_H
#define USERS_H

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

bool canModifyUser(const QByteArray &uId, quint32 myRank, bool equalAcceptable);

class ListUsers : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit ListUsers(QObject *parent = nullptr);
};

//------------------------------------

class LockUser : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit LockUser(QObject *parent = nullptr);
};

//------------------------------------

class CreateUser : public CmdObject
{
    Q_OBJECT

private:

    QString newName;
    QString dispName;
    QString email;

public:

    static QString cmdName();

    void clear();
    void procIn(const QByteArray &binIn, quint8 dType);

    explicit CreateUser(QObject *parent = nullptr);
};

//-----------------------------------

class RemoveUser : public CmdObject
{
    Q_OBJECT

private:

    QByteArray uId;

    void rm();
    void ask();

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RemoveUser(QObject *parent = nullptr);
};

//-----------------------------------

class ChangeUserRank : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ChangeUserRank(QObject *parent = nullptr);
};

//---------------------------------

class ChangePassword : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ChangePassword(QObject *parent = nullptr);
};

//-------------------------------

class ChangeUsername : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ChangeUsername(QObject *parent = nullptr);
};

//-----------------------------

class ChangeDispName : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ChangeDispName(QObject *parent = nullptr);
};

//-----------------------------

class OverWriteEmail : public CmdObject
{
    Q_OBJECT

protected:

    void procArgs(const QString &uName, const QString &newEmail, bool sameRank);

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit OverWriteEmail(QObject *parent = nullptr);
};

//-----------------------------

class ChangeEmail : public OverWriteEmail
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ChangeEmail(QObject *parent = nullptr);
};

//-----------------------------

class PasswordChangeRequest : public CmdObject
{
    Q_OBJECT

protected:

    virtual void exec(const QByteArray &uId, bool req);

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit PasswordChangeRequest(QObject *parent = nullptr);
};

//----------------------------

class NameChangeRequest : public PasswordChangeRequest
{
    Q_OBJECT

private:

    void exec(const QByteArray &uId, bool req);

public:

    static QString cmdName();

    explicit NameChangeRequest(QObject *parent = nullptr);
};

#endif // USERS_H
