#ifndef CHANNELS_H
#define CHANNELS_H

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

QByteArray createChMemberAsyncFrame(quint64 chId, const QByteArray &userId, bool invite, quint8 level, const QString &userName, const QString &dispName, const QString &chName);

class CreateChannel : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, uchar dType);

    explicit CreateChannel(QObject *parent = nullptr);
};

//-----------------------

class RemoveChannel : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RemoveChannel(QObject *parent = nullptr);
};

//----------------------

class RenameChannel : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RenameChannel(QObject *parent = nullptr);
};

//------------------------

class SetActiveState : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit SetActiveState(QObject *parent = nullptr);
};

//-------------------------

class CreateSubCh : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit CreateSubCh(QObject *parent = nullptr);
};

//-------------------------

class RemoveSubCh : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RemoveSubCh(QObject *parent = nullptr);
};

//--------------------------

class RenameSubCh : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RenameSubCh(QObject *parent = nullptr);
};

//------------------------

class ListSubCh : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ListSubCh(QObject *parent = nullptr);
};

//--------------------------

class ListChannels : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ListChannels(QObject *parent = nullptr);
};

//----------------------------

class SearchChannels : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit SearchChannels(QObject *parent = nullptr);
};

//------------------------------

class InviteToCh : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit InviteToCh(QObject *parent = nullptr);
};

//-------------------------------

class DeclineChInvite : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit DeclineChInvite(QObject *parent = nullptr);
};

//-------------------------------

class AcceptChInvite : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit AcceptChInvite(QObject *parent = nullptr);
};

//------------------------------

class RemoveChMember : public CmdObject
{
    Q_OBJECT

    bool allowMemberDel(const QByteArray &uId, quint64 chId);

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit RemoveChMember(QObject *parent = nullptr);
};

//------------------------------

class SetMemberLevel : public CmdObject
{
    Q_OBJECT

    bool allowLevelChange(const QByteArray &uId, int newLevel, quint64 chId);

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, uchar dType);

    explicit SetMemberLevel(QObject *parent = nullptr);
};

//------------------------------

class SetSubAcessLevel : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit SetSubAcessLevel(QObject *parent = nullptr);
};

//-----------------------------

class ListMembers : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ListMembers(QObject *parent = nullptr);
};

//-----------------------------

class OwnerOverride : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit OwnerOverride(QObject *parent = nullptr);
};

#endif // CHANNELS_H
