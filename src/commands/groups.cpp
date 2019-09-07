#include "groups.h"

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

ListGroups::ListGroups(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_GROUPS, QStringList() << COLUMN_GRNAME << COLUMN_HOST_RANK, false);
}

CreateGroup::CreateGroup(QObject *parent)   : InternCommand(parent) {}
RemoveGroup::RemoveGroup(QObject *parent)   : InternCommand(parent) {}
TransGroup::TransGroup(QObject *parent)     : InternCommand(parent) {}
SetGroupRank::SetGroupRank(QObject *parent) : InternCommand(parent) {}
RenameGroup::RenameGroup(QObject *parent)   : InternCommand(parent) {}

QString ListGroups::cmdName()   {return "ls_groups";}
QString CreateGroup::cmdName()  {return "add_group";}
QString RemoveGroup::cmdName()  {return "rm_group";}
QString TransGroup::cmdName()   {return "trans_group";}
QString SetGroupRank::cmdName() {return "set_group_rank";}
QString RenameGroup::cmdName()  {return "rename_group";}

void CreateGroup::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     grName = getParam("-name", args);

        if (grName.isEmpty())
        {
            errTxt("err: The group name argument (-name) was not found or is empty.\n");
        }
        else if (!validGroupName(grName))
        {
            errTxt("err: The group name must be 1-12 chars long and contain no spaces.\n");
        }
        else if (groupExists(grName))
        {
            errTxt("err: The requested group already exists.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::PUSH, TABLE_GROUPS);
            db.addColumn(COLUMN_GRNAME, grName);
            db.addColumn(COLUMN_HOST_RANK, 2);
            db.exec();
        }
    }
}

void RemoveGroup::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     grName = getParam("-name", args);

        if (grName.isEmpty())
        {
            errTxt("err: The group name (-name) argument was not found or is empty.\n");
        }
        else if (noCaseMatch(ROOT_USER, grName))
        {
            errTxt("err: '" + QString(ROOT_USER) + "' is a protected group, you cannot delete it.\n");
        }
        else if (!validGroupName(grName))
        {
            errTxt("err: Invalid group name.\n");
        }
        else if (!groupExists(grName))
        {
            errTxt("err: No such group found in the database.\n");
        }
        else if (noCaseMatch(grName, initGroup()))
        {
            errTxt("err: '" + grName + "' is the initial group for new users, unable to delete it.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, grName))
        {
            errTxt("err: The target group out ranks or is equal to your own group. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::DEL, TABLE_GROUPS);
            db.addCondition(COLUMN_GRNAME, grName);

            if (!db.exec())
            {
                errTxt("err: Unable to delete group. some user accounts might still be attached to it.\n");
            }
        }
    }
}

void TransGroup::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 4);
        QString     src  = getParam("-src", args);
        QString     dst  = getParam("-dst", args);

        if (src.isEmpty())
        {
            errTxt("err: The source group (-src) argument was not found or is empty.\n");
        }
        else if (dst.isEmpty())
        {
            errTxt("err: The destination group (-dst) argument was not found or is empty.\n");
        }
        else if (noCaseMatch(src, ROOT_USER) || noCaseMatch(dst, ROOT_USER))
        {
            errTxt("err: Unable to transfer to or from protected group '" + QString(ROOT_USER) + "'\n");
        }
        else if (!validGroupName(src))
        {
            errTxt("err: Invalid source group name.\n");
        }
        else if (!validGroupName(dst))
        {
            errTxt("err: Invalid destination group name.\n");
        }
        else if (!groupExists(src))
        {
            errTxt("err: '" + src + "' does not exists.\n");
        }
        else if (!groupExists(dst))
        {
            errTxt("err: '" + dst + "' does not exists.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, src))
        {
            errTxt("err: The source group out ranks your own group. access denied.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, dst))
        {
            errTxt("err: The destination group out ranks or is equal to your own group. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_GRNAME, dst);
            db.addCondition(COLUMN_GRNAME, src);
            db.exec();

            emit backendDataOut(ASYNC_GRP_TRANS, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void SetGroupRank::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 4);
        QString     grName = getParam("-name", args);
        QString     rank   = getParam("-rank", args);

        if (grName.isEmpty())
        {
            errTxt("err: The group name (-name) argument was not found or is empty.\n");
        }
        else if (noCaseMatch(ROOT_USER, grName))
        {
            errTxt("err: '" + QString(ROOT_USER) + "' is a protected group, you cannot change it.\n");
        }
        else if (!validGroupName(grName))
        {
            errTxt("err: Invalid group name.\n");
        }
        else if (!isInt(rank))
        {
            errTxt("err: Invalid rank.\n");
        }
        else if (rank.toUInt() < *sharedObjs->hostRank)
        {
            errTxt("err: you cannot set a rank higher than your own.\n");
        }
        else if (!groupExists(grName))
        {
            errTxt("err: Group name '" + grName + "' does not exists.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, grName))
        {
            errTxt("err: The target group out ranks or is equal to your own group. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_GROUPS);
            db.addColumn(COLUMN_HOST_RANK, rank.toUInt());
            db.addCondition(COLUMN_GRNAME, grName);
            db.exec();

            emit backendDataOut(ASYNC_GROUP_UPDATED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}

void RenameGroup::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList args    = parseArgs(binIn, 4);
        QString     name    = getParam("-name", args);
        QString     newName = getParam("-new_name", args);

        if (name.isEmpty())
        {
            errTxt("err: The group name (-name) argument was not found or is empty.\n");
        }
        else if (newName.isEmpty())
        {
            errTxt("err: The new name (-new_name) argument was not found or is empty.\n");
        }
        else if (noCaseMatch(name, ROOT_USER))
        {
            errTxt("err: Cannot rename protected group '" + QString(ROOT_USER) + "'\n");
        }
        else if (noCaseMatch(newName, ROOT_USER))
        {
            errTxt("err: Cannot use '" + QString(ROOT_USER) + "' for a new name. it is reserved.\n");
        }
        else if (!validGroupName(name))
        {
            errTxt("err: Invalid group name.\n");
        }
        else if (!validGroupName(newName))
        {
            errTxt("err: Invalid new group name. the group name must be between 1-12 chars long and contain no spaces.\n");
        }
        else if (!groupExists(name))
        {
            errTxt("err: Group name '" + name + "' does not exists.\n");
        }
        else if (groupExists(newName))
        {
            errTxt("err: Group name '" + newName + "' already exists.\n");
        }
        else if (!checkRank(*sharedObjs->groupName, name))
        {
            errTxt("err: The target group out ranks or is equal to your own group. access denied.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_GROUPS);
            db.addColumn(COLUMN_GRNAME, newName);
            db.addCondition(COLUMN_GRNAME, name);
            db.exec();

            args.clear();
            args.append("-src");
            args.append("'" + escapeChars(name, '\\', '\'') + "'");
            args.append("-dst");
            args.append("'" + escapeChars(newName, '\\', '\'') + "'");

            emit backendDataOut(ASYNC_GROUP_RENAMED, toTEXT(args.join(' ')), PUB_IPC_WITH_FEEDBACK);
        }
    }
}
