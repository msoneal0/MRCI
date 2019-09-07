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

CreateChannel::CreateChannel(QObject *parent)       : InternCommand(parent) {}
RemoveChannel::RemoveChannel(QObject *parent)       : InternCommand(parent) {}
RenameChannel::RenameChannel(QObject *parent)       : InternCommand(parent) {}
SetActiveState::SetActiveState(QObject *parent)     : InternCommand(parent) {}
CreateSubCh::CreateSubCh(QObject *parent)           : InternCommand(parent) {}
RemoveSubCh::RemoveSubCh(QObject *parent)           : InternCommand(parent) {}
RenameSubCh::RenameSubCh(QObject *parent)           : InternCommand(parent) {}
InviteToCh::InviteToCh(QObject *parent)             : InternCommand(parent) {}
DeclineChInvite::DeclineChInvite(QObject *parent)   : InternCommand(parent) {}
AcceptChInvite::AcceptChInvite(QObject *parent)     : InternCommand(parent) {}
RemoveChMember::RemoveChMember(QObject *parent)     : InternCommand(parent) {}
SetMemberLevel::SetMemberLevel(QObject *parent)     : InternCommand(parent) {}
SetSubAcessLevel::SetSubAcessLevel(QObject *parent) : InternCommand(parent) {}
OwnerOverride::OwnerOverride(QObject *parent)       : InternCommand(parent) {}

ListChannels::ListChannels(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CH_MEMBERS, QStringList() << COLUMN_CHANNEL_ID << COLUMN_CHANNEL_NAME << COLUMN_PENDING_INVITE << COLUMN_ACCESS_LEVEL << COLUMN_USERNAME, false);
}

ListSubCh::ListSubCh(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_SUB_CHANNELS, QStringList() << COLUMN_CHANNEL_NAME << COLUMN_CHANNEL_ID << COLUMN_SUB_CH_ID << COLUMN_SUB_CH_NAME << COLUMN_LOWEST_LEVEL << COLUMN_ACTIVE_UPDATE, false);
}

SearchChannels::SearchChannels(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CHANNELS, QStringList() << COLUMN_CHANNEL_ID << COLUMN_CHANNEL_NAME, false);
}

ListMembers::ListMembers(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CH_MEMBERS, QStringList() << COLUMN_CHANNEL_ID << COLUMN_CHANNEL_NAME << COLUMN_PENDING_INVITE << COLUMN_ACCESS_LEVEL << COLUMN_USERNAME, false);
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

void ListChannels::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(binIn);

    if (moreInputEnabled())
    {
        TableViewer::procBin(sharedObjs, binIn, dType);
    }
    else
    {
        TableViewer::procBin(sharedObjs, toTEXT("-" + QString(COLUMN_USERNAME) + " " + *sharedObjs->userName), dType);
    }
}

void ListSubCh::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     chName = getParam("-ch_name", args);

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else
        {
            if (channelAccessLevel(sharedObjs, getChId(chName)) > REGULAR)
            {
                TableViewer::procBin(sharedObjs, toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName + " -" + QString(COLUMN_LOWEST_LEVEL) + " " + QString::number(PUBLIC)), dType);
            }
            else
            {
                TableViewer::procBin(sharedObjs, toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName), dType);
            }
        }
    }
}

void SearchChannels::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        if (moreInputEnabled())
        {
            TableViewer::procBin(sharedObjs, binIn, dType);
        }
        else
        {
            QStringList args = parseArgs(binIn, 4);
            QString     name = getParam("-name", args);
            QString     chId = getParam("-id", args);

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
                TableViewer::procBin(sharedObjs, toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + name), dType);
            }
            else if (!chId.isEmpty())
            {
                TableViewer::procBin(sharedObjs, toTEXT("-" + QString(COLUMN_CHANNEL_ID) + " " + chId), dType);
            }
            else
            {
                TableViewer::procBin(sharedObjs, QByteArray(), dType);
            }
        }
    }
}

void ListMembers::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        if (moreInputEnabled())
        {
            TableViewer::procBin(sharedObjs, binIn, dType);
        }
        else
        {
            QStringList args     = parseArgs(binIn, 2);
            QString     chName   = getParam("-ch_name", args);
            QString     userFind = getParam("-find", args);

            if (chName.isEmpty())
            {
                errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
            }
            else if (!validChName(chName))
            {
                errTxt("err: Invalid channel name.\n");
            }
            else if (channelAccessLevel(sharedObjs, chName) > REGULAR)
            {
                errTxt("err: You are not currently a member of the channel: '" + chName + "'\n");
            }
            else
            {
                QByteArray argsBa = toTEXT("-" + QString(COLUMN_CHANNEL_NAME) + " " + chName);

                if (!userFind.isEmpty())
                {
                    argsBa.append(" " + toTEXT("-" + QString(COLUMN_USERNAME) + " " + userFind));
                }

                TableViewer::procBin(sharedObjs, argsBa, dType);
            }
        }
    }
}

void CreateChannel::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     chName = getParam("-ch_name", args);

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name. it must be between 4-32 chars long and contain no spaces.\n");
        }
        else if (channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' already exists.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::PUSH, TABLE_CHANNELS);
            db.addColumn(COLUMN_CHANNEL_NAME, chName);
            db.exec();

            quint64 chId = getChId(chName);

            db.setType(Query::PUSH, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_CHANNEL_ID, chId);
            db.addColumn(COLUMN_USERNAME, *sharedObjs->userName);
            db.addColumn(COLUMN_CHANNEL_NAME, chName);
            db.addColumn(COLUMN_ACCESS_LEVEL, OWNER);
            db.addColumn(COLUMN_PENDING_INVITE, false);
            db.exec();

            args.append("-user");
            args.append("'" + escapeChars(*sharedObjs->userName, '\\', '\'') + "'");
            args.append("-level");
            args.append(QString::number(OWNER));
            args.append("-ch_id");
            args.append(QString::number(chId));

            emit backendDataOut(ASYNC_NEW_CH_MEMBER, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RemoveChannel::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     chName = getParam("-ch_name", args);

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!validChName(chName))
        {
            errTxt("err: Invalid channel name.\n");
        }
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(sharedObjs, chName) != OWNER)
        {
            errTxt("err: Only the channel owner can delete it.\n");
        }
        else
        {
            quint64 id = getChId(chName);

            Query db(this);

            db.setType(Query::DEL, TABLE_CHANNELS);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.exec();

            args.append("-ch_id");
            args.append(QString::number(id));

            emit backendDataOut(ASYNC_DEL_CH, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RenameChannel::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 4);
        QString     chName  = getParam("-ch_name", args);
        QString     newName = getParam("-new_name", args);

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
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(sharedObjs, chName) != OWNER)
        {
            errTxt("err: Only the channel owner can rename it.\n");
        }
        else if (channelExists(newName))
        {
            errTxt("err: Channel name '" + newName + "' already exists.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_CHANNELS);
            db.addColumn(COLUMN_CHANNEL_NAME, newName);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.exec();

            emit backendDataOut(ASYNC_RENAME_CH, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void SetActiveState::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 6);
        QString     chName  = getParam("-ch_name", args);
        QString     subName = getParam("-sub_name", args);
        QString     state   = getParam("-state", args);

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
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(sharedObjs, chName) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chName, subName))
        {
            errTxt("err: Sub-channel name '" + chName + "' does not exists.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_ACTIVE_UPDATE, static_cast<bool>(state.toInt()));
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.addCondition(COLUMN_SUB_CH_NAME, subName);
            db.exec();

            if (globalActiveFlag())
            {
                mainTxt("warning: The host currently have the global active update flag set so setting this flag at the sub-channel level does nothing.\n");
            }
            else
            {
                emit backendDataOut(ASYNC_CH_ACT_FLAG, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
            }
        }
    }
}

void CreateSubCh::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 4);
        QString     chName  = getParam("-ch_name", args);
        QString     subName = getParam("-sub_name", args);
        int         subId   = 0;

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
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (channelAccessLevel(sharedObjs, chName) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (channelSubExists(chName, subName))
        {
            errTxt("err: Sub-channel name '" + subName + "' already exists.\n");
        }
        else if (!genSubId(chName, &subId))
        {
            errTxt("err: This channel has reached the maximum amount sub-channels it can have (" + QString::number(maxSubChannels()) + ").\n");
        }
        else
        {
            quint64 chId = getChId(chName);

            Query db(this);

            db.setType(Query::PUSH, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_SUB_CH_NAME, subName);
            db.addColumn(COLUMN_SUB_CH_ID, subId);
            db.addColumn(COLUMN_LOWEST_LEVEL, REGULAR);
            db.addColumn(COLUMN_CHANNEL_NAME, chName);
            db.addColumn(COLUMN_CHANNEL_ID, chId);
            db.addColumn(COLUMN_ACTIVE_UPDATE, false);
            db.exec();

            args.append("-ch_id");
            args.append(QString::number(chId));
            args.append("-level");
            args.append(QString::number(REGULAR));
            args.append("-sub_id");
            args.append(QString::number(subId));

            emit backendDataOut(ASYNC_NEW_SUB_CH, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RemoveSubCh::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 4);
        QString     chName  = getParam("-ch_name", args);
        QString     subName = getParam("-sub_name", args);

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
        else if (channelAccessLevel(sharedObjs, chName) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chName, subName))
        {
            errTxt("err: Sub-channel name '" + subName + "' does not exists.\n");
        }
        else
        {
            quint64 chId  = getChId(chName);
            uchar   subId = getSubId(chName, subName);

            Query db(this);

            db.setType(Query::DEL, TABLE_SUB_CHANNELS);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.addCondition(COLUMN_SUB_CH_NAME, subName);
            db.exec();

            args.append("-ch_id");
            args.append(QString::number(chId));
            args.append("-sub_id");
            args.append(QString::number(subId));

            emit backendDataOut(ASYNC_RM_SUB_CH, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RenameSubCh::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 6);
        QString     chName  = getParam("-ch_name", args);
        QString     subName = getParam("-sub_name", args);
        QString     newName = getParam("-new_name", args);

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
        else if (channelAccessLevel(sharedObjs, chName) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chName, subName))
        {
            errTxt("err: Sub-channel name '" + subName + "' does not exists.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_SUB_CH_NAME, newName);
            db.addCondition(COLUMN_SUB_CH_NAME, subName);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.exec();

            emit backendDataOut(ASYNC_RENAME_SUB_CH, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void InviteToCh::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 4);
        QString     chName = getParam("-ch_name", args);
        QString     uName  = getParam("-user", args);

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
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!userExists(uName))
        {
            errTxt("err: User name '" + uName + "' does not exists.\n");
        }
        else if (channelAccessLevel(sharedObjs, chName) > OFFICER)
        {
            errTxt("err: Access denied.\n");
        }
        else if (inviteExists(uName, chName))
        {
            errTxt("err: User name '" + uName + "' already has an invitation to channel '" + chName + ".'\n");
        }
        else if (channelAccessLevel(uName, chName) < PUBLIC)
        {
            errTxt("err: User name '" + uName + "' is already a member of the requested channel '" + chName + ".'\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::PUSH, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_USERNAME, uName);
            db.addColumn(COLUMN_CHANNEL_NAME, chName);
            db.addColumn(COLUMN_CHANNEL_ID, getChId(chName));
            db.addColumn(COLUMN_PENDING_INVITE, true);
            db.addColumn(COLUMN_ACCESS_LEVEL, REGULAR);
            db.exec();

            emit backendDataOut(ASYNC_INVITED_TO_CH, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void DeclineChInvite::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     chName = getParam("-ch_name", args);

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!inviteExists(*sharedObjs->userName, chName))
        {
            errTxt("err: You don't currently have an invitation to channel '" + chName + ".'\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::DEL, TABLE_CH_MEMBERS);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
            db.exec();

            args.append("-ch_id");
            args.append(QString::number(getChId(chName)));
            args.append("-user");
            args.append("'" + escapeChars(*sharedObjs->userName, '\\', '\'') + "'");

            emit backendDataOut(ASYNC_RM_CH_MEMBER, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void AcceptChInvite::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     chName = getParam("-ch_name", args);

        if (chName.isEmpty())
        {
            errTxt("err: The channel name (-ch_name) argument was not found or is empty.\n");
        }
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!inviteExists(*sharedObjs->userName, chName))
        {
            errTxt("err: You don't currently have an invitation to channel '" + chName + ".'\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_PENDING_INVITE, false);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
            db.exec();

            args.append("-user");
            args.append("'" + escapeChars(*sharedObjs->userName, '\\', '\'') + "'");

            emit backendDataOut(ASYNC_INVITE_ACCEPTED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RemoveChMember::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 4);
        QString     chName = getParam("-ch_name", args);
        QString     uName  = getParam("-user", args);

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
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!userExists(uName))
        {
            errTxt("err: User name '" + uName + "' does not exists.\n");
        }
        else if (!allowMemberDel(sharedObjs, uName, chName))
        {
            errTxt("err: Access denied.\n");
        }
        else
        {
            quint64 id = getChId(chName);

            Query db(this);

            db.setType(Query::DEL, TABLE_CH_MEMBERS);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.addCondition(COLUMN_USERNAME, uName);
            db.exec();

            args.append("-ch_id");
            args.append(QString::number(id));

            emit backendDataOut(ASYNC_RM_CH_MEMBER, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void SetMemberLevel::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 6);
        QString     chName = getParam("-ch_name", args);
        QString     uName  = getParam("-user", args);
        QString     level  = getParam("-level", args);

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
        else if (!channelExists(chName))
        {
            errTxt("err: Channel name '" + chName + "' does not exists.\n");
        }
        else if (!userExists(uName))
        {
            errTxt("err: User name '" + uName + "' does not exists.\n");
        }
        else if (!allowLevelChange(sharedObjs, level.toInt(), chName))
        {
            errTxt("err: Access denied.\n");
        }
        else if (channelAccessLevel(uName, chName) < PUBLIC)
        {
            errTxt("err: The target user '" + uName + "' is not a member of the channel.\n");
        }
        else
        {
            quint64 id = getChId(chName);

            Query db(this);

            db.setType(Query::PULL, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_USERNAME);
            db.addCondition(COLUMN_ACCESS_LEVEL, OWNER);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.exec();

            QString owner = db.getData(COLUMN_USERNAME).toString();

            db.setType(Query::UPDATE, TABLE_CH_MEMBERS);
            db.addColumn(COLUMN_ACCESS_LEVEL, level.toInt());
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.addCondition(COLUMN_USERNAME, uName);
            db.exec();

            args.append("-ch_id");
            args.append(QString::number(id));

            emit backendDataOut(ASYNC_MEM_LEVEL_CHANGED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);

            if (level.toInt() == OWNER)
            {
                db.setType(Query::UPDATE, TABLE_CH_MEMBERS);
                db.addColumn(COLUMN_ACCESS_LEVEL, ADMIN);
                db.addCondition(COLUMN_CHANNEL_NAME, chName);
                db.addCondition(COLUMN_USERNAME, owner);
                db.exec();

                args.clear();
                args.append("-user");
                args.append("'" + escapeChars(owner, '\\', '\'') + "'");
                args.append("-ch_name");
                args.append("'" + escapeChars(chName, '\\', '\'') + "'");
                args.append("-ch_id");
                args.append(QString::number(id));
                args.append("-level");
                args.append(QString::number(ADMIN));

                emit backendDataOut(ASYNC_MEM_LEVEL_CHANGED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
            }
        }
    }
}

void SetSubAcessLevel::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 6);
        QString     chName  = getParam("-ch_name", args);
        QString     subName = getParam("-sub_name", args);
        QString     level   = getParam("-level", args);

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
        else if (channelAccessLevel(sharedObjs, chName) > ADMIN)
        {
            errTxt("err: Access denied.\n");
        }
        else if (!channelSubExists(chName, subName))
        {
            errTxt("err: Sub-channel name '" + subName + "' does not exists.\n");
        }
        else
        {
            quint64 chId  = getChId(chName);
            uchar   subId = getSubId(chName, subName);

            Query db(this);

            db.setType(Query::UPDATE, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_LOWEST_LEVEL, level.toInt());
            db.addCondition(COLUMN_SUB_CH_NAME, subName);
            db.addCondition(COLUMN_CHANNEL_NAME, chName);
            db.exec();

            args.append("-ch_id");
            args.append(QString::number(chId));
            args.append("-sub_id");
            args.append(QString::number(subId));

            emit backendDataOut(ASYNC_SUB_CH_LEVEL_CHG, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void OwnerOverride::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 2);
        QString     state = getParam("-state", args);

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
            *rwSharedObjs->chOwnerOverride = state.toInt();
        }
    }
}
