#include "channels.h"

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

CreateChannel::CreateChannel(QObject *parent)       : CmdObject(parent) {}
RemoveChannel::RemoveChannel(QObject *parent)       : CmdObject(parent) {}
RenameChannel::RenameChannel(QObject *parent)       : CmdObject(parent) {}
SetActiveState::SetActiveState(QObject *parent)     : CmdObject(parent) {}
CreateSubCh::CreateSubCh(QObject *parent)           : CmdObject(parent) {}
RemoveSubCh::RemoveSubCh(QObject *parent)           : CmdObject(parent) {}
RenameSubCh::RenameSubCh(QObject *parent)           : CmdObject(parent) {}
InviteToCh::InviteToCh(QObject *parent)             : CmdObject(parent) {}
DeclineChInvite::DeclineChInvite(QObject *parent)   : CmdObject(parent) {}
AcceptChInvite::AcceptChInvite(QObject *parent)     : CmdObject(parent) {}
RemoveChMember::RemoveChMember(QObject *parent)     : CmdObject(parent) {}
SetMemberLevel::SetMemberLevel(QObject *parent)     : CmdObject(parent) {}
SetSubAcessLevel::SetSubAcessLevel(QObject *parent) : CmdObject(parent) {}
OwnerOverride::OwnerOverride(QObject *parent)       : CmdObject(parent) {}

ListChannels::ListChannels(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CH_MEMBERS, false);
    addTableColumn(TABLE_CH_MEMBERS, COLUMN_CHANNEL_ID);
    addTableColumn(TABLE_CHANNELS, COLUMN_CHANNEL_NAME);
    addTableColumn(TABLE_CH_MEMBERS, COLUMN_PENDING_INVITE);
    addTableColumn(TABLE_CH_MEMBERS, COLUMN_ACCESS_LEVEL);
    addJointColumn(TABLE_CHANNELS, COLUMN_CHANNEL_ID);
    addJointColumn(TABLE_USERS, COLUMN_USER_ID);
}

ListSubCh::ListSubCh(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_SUB_CHANNELS, false);
    addTableColumn(TABLE_SUB_CHANNELS, COLUMN_SUB_CH_ID);
    addTableColumn(TABLE_SUB_CHANNELS, COLUMN_SUB_CH_NAME);
    addTableColumn(TABLE_SUB_CHANNELS, COLUMN_LOWEST_LEVEL);
    addTableColumn(TABLE_SUB_CHANNELS, COLUMN_ACTIVE_UPDATE);
}

SearchChannels::SearchChannels(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CHANNELS, false);
    addTableColumn(TABLE_CHANNELS, COLUMN_CHANNEL_ID);
    addTableColumn(TABLE_CHANNELS, COLUMN_CHANNEL_NAME);
}

ListMembers::ListMembers(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CH_MEMBERS, false);
    addTableColumn(TABLE_CH_MEMBERS, COLUMN_CHANNEL_ID);
    addTableColumn(TABLE_USERS, COLUMN_USERNAME);
    addTableColumn(TABLE_USERS, COLUMN_DISPLAY_NAME);
    addTableColumn(TABLE_CH_MEMBERS, COLUMN_PENDING_INVITE);
    addTableColumn(TABLE_CH_MEMBERS, COLUMN_ACCESS_LEVEL);
    addJointColumn(TABLE_USERS, COLUMN_USER_ID);
}

QString CreateChannel::cmdName()    {return "add_ch";}
QString RemoveChannel::cmdName()    {return "rm_ch";}
QString RenameChannel::cmdName()    {return "rename_ch";}
QString SetActiveState::cmdName()   {return "set_active_flag";}
QString CreateSubCh::cmdName()      {return "add_sub_ch";}
QString RemoveSubCh::cmdName()      {return "rm_sub_ch";}
QString RenameSubCh::cmdName()      {return "rename_sub_ch";}
QString ListChannels::cmdName()     {return "ls_chs";}
QString ListSubCh::cmdName()        {return "ls_sub_chs";}
QString SearchChannels::cmdName()   {return "find_ch";}
QString InviteToCh::cmdName()       {return "invite_to_ch";}
QString DeclineChInvite::cmdName()  {return "decline_ch";}
QString AcceptChInvite::cmdName()   {return "accept_ch";}
QString RemoveChMember::cmdName()   {return "remove_ch_member";}
QString SetMemberLevel::cmdName()   {return "set_member_level";}
QString SetSubAcessLevel::cmdName() {return "set_sub_ch_level";}
QString ListMembers::cmdName()      {return "ls_ch_members";}
QString OwnerOverride::cmdName()    {return "ch_owner_override";}

QByteArray createChMemberAsyncFrame(quint64 chId, const QByteArray &userId, bool invite, quint8 level, const QString &userName, const QString &dispName, const QString &chName)
{
    QByteArray ret;

    ret.append(wrInt(chId, 64));
    ret.append(userId);

    if (invite)
    {
        ret.append(wrInt(1, 8));
    }
    else
    {
        ret.append(wrInt(0, 8));
    }

    ret.append(wrInt(level, 8));
    ret.append(nullTermTEXT(userName));
    ret.append(nullTermTEXT(dispName));
    ret.append(nullTermTEXT(chName));

    return ret;
}

void ListChannels::procIn(const QByteArray &binIn, quint8 dType)
{
    Q_UNUSED(binIn)

    if (flags & MORE_INPUT)
    {
        TableViewer::procIn(binIn, dType);
    }
    else
    {
        TableViewer::procIn(toTEXT("-" + QString(COLUMN_USERNAME) + " " + rdStringFromBlock(userName, BLKSIZE_USER_NAME)), dType);
    }
}

void ListSubCh::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 2);
        auto chName = getParam("-ch_name", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > REGULAR)
            {
                TableViewer::procIn(toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName + " -" + QString(COLUMN_LOWEST_LEVEL) + " " + QString::number(PUBLIC)), dType);
            }
            else
            {
                TableViewer::procIn(toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName), dType);
            }
        }
    }
}

void SearchChannels::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            TableViewer::procIn(binIn, dType);
        }
        else
        {
            auto args = parseArgs(binIn, 4);
            auto name = getParam("-name", args);
            auto chId = getParam("-id", args);

            retCode = INVALID_PARAMS;

            if (!name.isEmpty() && !validChName(name))
            {
                errTxt("err: '" + name + "' is not a valid channel name.\n");
            }
            else if (!chId.isEmpty() && !isInt(chId))
            {
                errTxt("err: '" + chId + "' is not a valid channel id.\n");
            }
            else if (!name.isEmpty())
            {
                retCode = NO_ERRORS;

                TableViewer::procIn(toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + name), dType);
            }
            else if (!chId.isEmpty())
            {
                retCode = NO_ERRORS;

                TableViewer::procIn(toTEXT("-" + QString(COLUMN_CHANNEL_ID) + " " + chId), dType);
            }
            else
            {
                retCode = NO_ERRORS;

                TableViewer::procIn(QByteArray(), dType);
            }
        }
    }
}

void ListMembers::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            TableViewer::procIn(binIn, dType);
        }
        else
        {
            auto args     = parseArgs(binIn, 6);
            auto chName   = getParam("-ch_name", args);
            auto userFind = getParam("-user_name", args);
            auto dispFind = getParam("-disp_name", args);

            quint64 chId;

            retCode = INVALID_PARAMS;

            if (chName.isEmpty())
            {
                errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
            }
            else if (!validChName(chName))
            {
                errTxt("err: Invalid channel name.\n");
            }
            else if (!channelExists(chName, &chId))
            {
                errTxt("err: Channel name '" + chName + "' does not exists.\n");
            }
            else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > REGULAR)
            {
                errTxt("err: You are not currently a member of the channel: '" + chName + "'\n");
            }
            else
            {
                retCode = NO_ERRORS;

                auto argsBa = toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName);

                if (!userFind.isEmpty())
                {
                    argsBa.append(toTEXT(" -" + QString(COLUMN_USERNAME) + " " + userFind));
                }

                if (!dispFind.isEmpty())
                {
                    argsBa.append(toTEXT(" -" + QString(COLUMN_DISPLAY_NAME) + " " + dispFind));
                }

                TableViewer::procIn(argsBa, dType);
            }
        }
    }
}

void CreateChannel::procIn(const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 2);
        auto chName = getParam("-ch_name", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name. it must be between 4-32 chars long and contain no spaces.\n");
        }
        else if (channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' already exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::PUSH, TABLE_CHANNELS);
            db.addColumn(COLUMN_CHANNEL_NAME, chName);
            db.exec();

            auto uId   = rdFromBlock(userId, BLKSIZE_USER_ID);
            auto uName = rdStringFromBlock(userName, BLKSIZE_USER_NAME);
            auto dName = rdStringFromBlock(displayName, BLKSIZE_DISP_NAME);

            db.setType(Query::PUSH, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_CHANNEL_ID, chId);
            db.addColumn(COLUMN_USER_ID, uId);
            db.addColumn(COLUMN_ACCESS_LEVEL, OWNER);
            db.addColumn(COLUMN_PENDING_INVITE, false);
            db.exec();

            auto frame = createChMemberAsyncFrame(chId, uId, false, OWNER, uName, dName, chName);

            async(ASYNC_NEW_CH_MEMBER, PUB_IPC_WITH_FEEDBACK, frame);
        }
    }
}

void RemoveChannel::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 2);
        auto chName = getParam("-ch_name", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) != OWNER)
        {
            errTxt("err: Only the channel owner can delete it.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::DEL, TABLE_CHANNELS);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.exec();

            async(ASYNC_DEL_CH, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64));
        }
    }
}

void RenameChannel::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 4);
        auto chName  = getParam("-ch_name", args);
        auto newName = getParam("-new_name", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (newName.isEmpty())
        {
            errTxt("err: The new channel name (-new_name) was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validChName(newName))
        {
            errTxt("err: Invalid new channel name. it must be between 4-32 chars long and contain no spaces.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) != OWNER)
        {
            errTxt("err: Only the channel owner can rename it.\n");
        }
        else if (channelExists(newName))
        {
            errTxt("err: Channel name '" + newName + "' already exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::UPDATE, TABLE_CHANNELS);
            db.addColumn(COLUMN_CHANNEL_NAME, newName);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.exec();

            QByteArray frame = wrInt(chId, 64) + nullTermTEXT(newName);

            async(ASYNC_RENAME_CH, PUB_IPC_WITH_FEEDBACK, frame);
        }
    }
}

void SetActiveState::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 6);
        auto chName  = getParam("-ch_name", args);
        auto subName = getParam("-sub_name", args);
        auto state   = getParam("-state", args);

        quint64 chId;
        quint8  subId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (subName.isEmpty())
        {
            errTxt("err: The sub-channel name (-sub_name) was not found or is empty.\n");
        }
        else if (state.isEmpty())
        {
            errTxt("err: The active flag state (-state) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validChName(subName))
        {
            errTxt("err: Invalid sub-channel name.\n");
        }
        else if (!isBool(state))
        {
            errTxt("err: '" + state + "' is not a valid boolean value. it must be 0 or 1.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chId, subName, &subId))
        {
            errTxt("err: Sub-channel name '" + chName + "' does not exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::UPDATE, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_ACTIVE_UPDATE, static_cast<bool>(state.toInt()));
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_SUB_CH_ID, subId);
            db.exec();

            if (globalActiveFlag())
            {
                mainTxt("warning: The host currently have the global active update flag set so setting this flag at the sub-channel level does nothing.\n");
            }
            else
            {
                async(ASYNC_CH_ACT_FLAG, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + wrInt(subId, 8));
            }
        }
    }
}

void CreateSubCh::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 4);
        auto chName  = getParam("-ch_name", args);
        auto subName = getParam("-sub_name", args);

        quint64 chId;
        quint8  subId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (subName.isEmpty())
        {
            errTxt("err: The sub-channel name (-sub_name) was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validChName(subName))
        {
            errTxt("err: Invalid sub-channel name. it must be between 4-32 chars long and contain no spaces.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (channelSubExists(chId, subName))
        {
            errTxt("err: Sub-channel name '" + subName + "' already exists.\n");
        }
        else if (!genSubId(chId, &subId))
        {
            errTxt("err: This channel has reached the maximum amount sub-channels it can have (" + QString::number(maxSubChannels()) + ").\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::PUSH, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_SUB_CH_NAME, subName);
            db.addColumn(COLUMN_SUB_CH_ID, subId);
            db.addColumn(COLUMN_LOWEST_LEVEL, REGULAR);
            db.addColumn(COLUMN_CHANNEL_ID, chId);
            db.addColumn(COLUMN_ACTIVE_UPDATE, false);
            db.exec();

            auto frame = wrInt(chId, 64) + wrInt(subId, 8) + wrInt(REGULAR, 8) + wrInt(0, 8) + nullTermTEXT(subName);

            async(ASYNC_NEW_SUB_CH, PUB_IPC_WITH_FEEDBACK, frame);
        }
    }
}

void RemoveSubCh::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 4);
        auto chName  = getParam("-ch_name", args);
        auto subName = getParam("-sub_name", args);

        quint64 chId;
        quint8  subId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (subName.isEmpty())
        {
            errTxt("err: The sub-channel name (-sub_name) was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validChName(subName))
        {
            errTxt("err: Invalid sub-channel name.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chId, subName, &subId))
        {
            errTxt("err: Sub-channel name '" + subName + "' does not exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::DEL, TABLE_SUB_CHANNELS);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_SUB_CH_ID, subId);
            db.exec();

            async(ASYNC_RM_SUB_CH, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + wrInt(subId, 8));
        }
    }
}

void RenameSubCh::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 6);
        auto chName  = getParam("-ch_name", args);
        auto subName = getParam("-sub_name", args);
        auto newName = getParam("-new_name", args);

        quint64 chId;
        quint8  subId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (subName.isEmpty())
        {
            errTxt("err: The sub-channel name (-sub_name) argument was not found or is empty.\n");
        }
        else if (newName.isEmpty())
        {
            errTxt("err: The new sub-channel name (-new_name) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validChName(subName))
        {
            errTxt("err: Invalid sub-channel name.\n");
        }
        else if (!validChName(newName))
        {
            errTxt("err: Invalid new sub-channel name. it must be between 4-32 chars long and contain no spaces.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chId, subName, &subId))
        {
            errTxt("err: Sub-channel name '" + subName + "' does not exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::UPDATE, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_SUB_CH_NAME, newName);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_SUB_CH_ID, subId);
            db.exec();

            auto frame = wrInt(chId, 64) + wrInt(subId, 8) + nullTermTEXT(newName);

            async(ASYNC_RENAME_SUB_CH, PUB_IPC_WITH_FEEDBACK, frame);
        }
    }
}

void InviteToCh::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 4);
        auto chName = getParam("-ch_name", args);
        auto uName  = getParam("-user", args);

        QByteArray uId;
        quint64    chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (uName.isEmpty())
        {
            errTxt("err: The user name (-user) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid user name.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!userExists(uName, &uId))
        {
            errTxt("err: User name '" + uName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > OFFICER)
        {
            errTxt("err: Access denied.\n");
        }
        else if (inviteExists(uId, chId))
        {
            errTxt("err: User name '" + uName + "' already has an invitation to channel '" + chName + ".'\n");
        }
        else if (channelAccessLevel(uId, chId) < PUBLIC)
        {
            errTxt("err: User name '" + uName + "' is already a member of the requested channel '" + chName + ".'\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::PUSH, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_USER_ID, uId);
            db.addColumn(COLUMN_CHANNEL_ID, chId);
            db.addColumn(COLUMN_PENDING_INVITE, true);
            db.addColumn(COLUMN_ACCESS_LEVEL, REGULAR);
            db.exec();

            auto frame = createChMemberAsyncFrame(chId, uId, true, REGULAR, uName, getDispName(uId), chName);

            async(ASYNC_INVITED_TO_CH, PUB_IPC_WITH_FEEDBACK, frame);
        }
    }
}

void DeclineChInvite::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 2);
        auto chName = getParam("-ch_name", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!inviteExists(rdFromBlock(userId, BLKSIZE_USER_ID), chId))
        {
            errTxt("err: You don't currently have an invitation to channel '" + chName + ".'\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::DEL, TABLE_CH_MEMBERS);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_USER_ID, rdFromBlock(userId, BLKSIZE_USER_ID));
            db.exec();

            async(ASYNC_RM_CH_MEMBER, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + rdFromBlock(userId, BLKSIZE_USER_ID));
        }
    }
}

void AcceptChInvite::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 2);
        auto chName = getParam("-ch_name", args);

        quint64 chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!inviteExists(rdFromBlock(userId, BLKSIZE_USER_ID), chId))
        {
            errTxt("err: You don't currently have an invitation to channel '" + chName + ".'\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::UPDATE, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_PENDING_INVITE, false);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_USER_ID, rdFromBlock(userId, BLKSIZE_USER_ID));
            db.exec();

            async(ASYNC_INVITE_ACCEPTED, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + rdFromBlock(userId, BLKSIZE_USER_ID));
        }
    }
}

bool RemoveChMember::allowMemberDel(const QByteArray &uId, quint64 chId)
{
    auto myId        = rdFromBlock(userId, BLKSIZE_USER_ID);
    auto ret         = false;
    auto leaving     = (uId == myId);
    auto targetLevel = channelAccessLevel(uId, chId);
    auto myLevel     = channelAccessLevel(myId, chOwnerOverride, BLKSIZE_USER_ID);

    if (leaving && (myLevel == OWNER))
    {
        errTxt("err: The channel owner cannot leave the channel. please assign a new owner before doing so.\n");
    }
    else if (targetLevel == PUBLIC)
    {
        errTxt("err: The target user is not a member of the channel.\n");
    }
    else if (leaving)
    {
        ret = myLevel != PUBLIC;
    }
    else
    {
        ret = myLevel < targetLevel;
    }

    return ret;
}

void RemoveChMember::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 4);
        auto chName = getParam("-ch_name", args);
        auto uName  = getParam("-user", args);

        QByteArray uId;
        quint64    chId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (uName.isEmpty())
        {
            errTxt("err: The user name (-user) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid user name.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!userExists(uName, &uId))
        {
            errTxt("err: User name '" + uName + "' does not exists.\n");
        }
        else if (!allowMemberDel(uId, chId))
        {
            errTxt("err: Access denied.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::DEL, TABLE_CH_MEMBERS);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            async(ASYNC_RM_CH_MEMBER, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + uId);
        }
    }
}

bool SetMemberLevel::allowLevelChange(const QByteArray &uId, int newLevel, quint64 chId)
{
    auto targetLevel = channelAccessLevel(uId, chId);
    auto myLevel     = channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, BLKSIZE_USER_ID);

    return (newLevel >= myLevel) && (targetLevel > myLevel);
}

void SetMemberLevel::procIn(const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        auto args   = parseArgs(binIn, 6);
        auto chName = getParam("-ch_name", args);
        auto uName  = getParam("-user", args);
        auto level  = getParam("-level", args);

        quint64    chId;
        QByteArray uId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (uName.isEmpty())
        {
            errTxt("err: The user name (-user) argument was not found or is empty.\n");
        }
        else if (level.isEmpty())
        {
            errTxt("err: The privilege level (-level) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validUserName(uName))
        {
            errTxt("err: Invalid user name.\n");
        }
        else if (!validLevel(level, false))
        {
            errTxt("err: Invalid privilege level. it must be an integer between 1-4.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!userExists(uName, &uId))
        {
            errTxt("err: User name '" + uName + "' does not exists.\n");
        }
        else if (!allowLevelChange(uId, level.toInt(), chId))
        {
            errTxt("err: Access denied.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::PULL, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_USER_ID);
            db.addCondition(COLUMN_ACCESS_LEVEL, OWNER);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.exec();

            auto newLevel = level.toInt();
            auto owner    = db.getData(COLUMN_USER_ID).toByteArray();

            db.setType(Query::UPDATE, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_ACCESS_LEVEL, newLevel);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            async(ASYNC_MEM_LEVEL_CHANGED, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + uId + wrInt(newLevel, 8));

            if (level.toInt() == OWNER)
            {
                db.setType(Query::UPDATE, TABLE_CH_MEMBERS);
                db.addColumn(COLUMN_ACCESS_LEVEL, ADMIN);
                db.addCondition(COLUMN_CHANNEL_ID, chId);
                db.addCondition(COLUMN_USER_ID, owner);
                db.exec();

                async(ASYNC_MEM_LEVEL_CHANGED, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + owner + wrInt(ADMIN, 8));
            }
        }
    }
}

void SetSubAcessLevel::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args    = parseArgs(binIn, 6);
        auto chName  = getParam("-ch_name", args);
        auto subName = getParam("-sub_name", args);
        auto level   = getParam("-level", args);

        quint64 chId;
        quint8  subId;

        retCode = INVALID_PARAMS;

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (subName.isEmpty())
        {
            errTxt("err: The sub-channel name (-sub_name) argument was not found or is empty.\n");
        }
        else if (level.isEmpty())
        {
            errTxt("err: The privilege level (-level) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!validChName(subName))
        {
            errTxt("err: Invalid sub-channel name.\n");
        }
        else if (!validLevel(level, true))
        {
            errTxt("err: Invalid privilege level. it must be an integer between 1-5.\n");
        }
        else if (!channelExists(chName, &chId))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(rdFromBlock(userId, BLKSIZE_USER_ID), chOwnerOverride, chId) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chId, subName, &subId))
        {
            errTxt("err: Sub-channel name '" + subName + "' does not exists.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::UPDATE, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_LOWEST_LEVEL, level.toInt());
            db.addCondition(COLUMN_SUB_CH_ID, subId);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.exec();

            async(ASYNC_SUB_CH_LEVEL_CHG, PUB_IPC_WITH_FEEDBACK, wrInt(chId, 64) + wrInt(subId, 8) + wrInt(level.toInt(), 8));
        }
    }
}

void OwnerOverride::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args  = parseArgs(binIn, 2);
        auto state = getParam("-state", args);

        retCode = INVALID_PARAMS;

        if (state.isEmpty())
        {
            errTxt("err: The flag state (-state) argument was not found or is empty.\n");
        }
        else if (!isBool(state))
        {
            errTxt("err: Invalid bool value for -state, it must be 1 or 0.\n");
        }
        else
        {
            retCode = NO_ERRORS;
            
            wr8BitToBlock(static_cast<quint8>(state.toUInt()), chOwnerOverride);
        }
    }
}
