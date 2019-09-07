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
#include "table_viewer.h"

class ListUsers : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit ListUsers(QObject *parent = nullptr);
};

//------------------------------------

class LockUser : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit LockUser(QObject *parent = nullptr);
};

//------------------------------------

class CreateUser : public InternCommand
{
    Q_OBJECT

private:

    QString newName;
    QString dispName;
    QString email;

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit CreateUser(QObject *parent = nullptr);
};

//-----------------------------------

class RemoveUser : public InternCommand
{
    Q_OBJECT

private:

    QString uName;

    void rm();
    void ask();

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit RemoveUser(QObject *parent = nullptr);
};

//-----------------------------------

class ChangeGroup : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ChangeGroup(QObject *parent = nullptr);
};

//---------------------------------

class ChangePassword : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ChangePassword(QObject *parent = nullptr);
};

//-------------------------------

class ChangeUsername : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ChangeUsername(QObject *parent = nullptr);
};

//-----------------------------

class ChangeDispName : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ChangeDispName(QObject *parent = nullptr);
};

//-----------------------------

class OverWriteEmail : public InternCommand
{
    Q_OBJECT

protected:

    void procArgs(const QString &uName, const QString &newEmail, bool sameRank, const SharedObjs *sharedObjs);

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit OverWriteEmail(QObject *parent = nullptr);
};

//-----------------------------

class ChangeEmail : public OverWriteEmail
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ChangeEmail(QObject *parent = nullptr);
};

//-----------------------------

class PasswordChangeRequest : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit PasswordChangeRequest(QObject *parent = nullptr);
};

//----------------------------

class NameChangeRequest : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit NameChangeRequest(QObject *parent = nullptr);
};

#endif // USERS_H
