#include "common.h"

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

QString sessionCountShareKey()
{
    return QString(APP_NAME) + ".SessionCount";
}

QString boolStr(bool state)
{
    QString ret;

    if (state) ret = "true";
    else       ret = "false";

    return ret;
}

uint rdSessionLoad()
{
    uint ret = 0;

    QSharedMemory mem(sessionCountShareKey());

    if (mem.attach(QSharedMemory::ReadOnly))
    {
        mem.lock();

        memcpy(&ret, mem.data(), 4);

        mem.unlock();
        mem.detach();
    }

    return ret;
}

void wrSessionLoad(uint value)
{
    QSharedMemory mem(sessionCountShareKey());

    if (mem.attach(QSharedMemory::ReadWrite))
    {
        mem.lock();

        memcpy(mem.data(), &value, 4);

        mem.unlock();
        mem.detach();
    }
}

QString genSerialNumber()
{
    return QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz");
}

void serializeThread(QThread *thr)
{
    thr->setObjectName(genSerialNumber());
}

QByteArray wrFrame(quint16 cmdId, const QByteArray &data, uchar dType)
{
    QByteArray cmdBa   = wrInt(cmdId, 16);
    QByteArray typeBa  = wrInt(dType, 8);
    QByteArray sizeBa  = wrInt(data.size(), MAX_FRAME_BITS);

    return typeBa + cmdBa + sizeBa + data;
}

QByteArray wrInt(quint64 num, int numOfBits)
{
    QByteArray ret(numOfBits / 8, static_cast<char>(0));

    num = qToLittleEndian(num);

    memcpy(ret.data(), &num, static_cast<size_t>(ret.size()));

    return ret;
}

QByteArray wrInt(qint64 num, int numOfBits)
{
    return wrInt(static_cast<quint64>(num), numOfBits);
}

QByteArray wrInt(int num, int numOfBits)
{
    return wrInt(static_cast<quint64>(num), numOfBits);
}

QByteArray wrInt(uint num, int numOfBits)
{
    return wrInt(static_cast<quint64>(num), numOfBits);
}

QByteArray toFILE_INFO(const QFileInfo &info)
{
    // this function converts some information extracted from a QFileInfo object to
    // a FILE_INFO frame.

    // format: [1byte(flags)][8bytes(createTime)][8bytes(modTime)][8bytes(fileSize)]
    //         [TEXT(fileName)][TEXT(symLinkTarget)]

    //         note: the TEXT strings are 16bit NULL terminated meaning 2 bytes of 0x00
    //               indicate the end of the string.

    //         note: the integer data found in flags, modTime, createTime and fileSize
    //               are formatted in little endian byte order (unsigned).

    char flags = 0;

    if (info.isFile())       flags |= IS_FILE;
    if (info.isDir())        flags |= IS_DIR;
    if (info.isSymLink())    flags |= IS_SYMLNK;
    if (info.isReadable())   flags |= CAN_READ;
    if (info.isWritable())   flags |= CAN_WRITE;
    if (info.isExecutable()) flags |= CAN_EXE;
    if (info.exists())       flags |= EXISTS;

    QByteArray ret;
    QByteArray strTerm(2, 0);

    ret.append(flags);
    ret.append(wrInt(info.birthTime().toMSecsSinceEpoch(), 64));
    ret.append(wrInt(info.lastModified().toMSecsSinceEpoch(), 64));
    ret.append(wrInt(info.size(), 64));
    ret.append(toTEXT(info.fileName()) + strTerm);
    ret.append(toTEXT(info.symLinkTarget() + strTerm));

    return ret;
}

QByteArray toFILE_INFO(const QString &path)
{
    return toFILE_INFO(QFileInfo(path));
}

QByteArray toPEER_INFO(const SharedObjs *sharedObjs)
{
    return *sharedObjs->sessionId +
           *sharedObjs->userId    +
            fixedToTEXT(*sharedObjs->userName, 24) +
            fixedToTEXT(*sharedObjs->appName, 64) +
            fixedToTEXT(*sharedObjs->displayName, 32);
}

QByteArray toNEW_CMD(quint16 cmdId, const QString &cmdName, ExternCommand *cmdObj)
{
    QByteArray idBa  = wrInt(cmdId, 16);
    QByteArray genBa = wrInt(0, 8);

    if (cmdObj->handlesGenfile())
    {
        genBa = wrInt(1, 8);
    }

    return idBa + genBa + fixedToTEXT(cmdName, 64) + fixedToTEXT(cmdObj->libText(), 64);
}

QByteArray toMY_INFO(const SharedObjs *sharedObjs)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_EMAIL);
    db.addColumn(COLUMN_EMAIL_VERIFIED);
    db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
    db.exec();

    QByteArray confirmed;

    if (db.getData(COLUMN_EMAIL_VERIFIED).toBool())
    {
        confirmed.append(static_cast<char>(0x01));
    }
    else
    {
        confirmed.append(static_cast<char>(0x00));
    }

    return toPEER_INFO(sharedObjs) +
           fixedToTEXT(db.getData(COLUMN_EMAIL).toString(), 64) +
           fixedToTEXT(*sharedObjs->groupName, 12) +
           confirmed;
}

QByteArray toPEER_STAT(const QByteArray &sesId, const QByteArray &chIds, bool isDisconnecting)
{
    if (isDisconnecting)
    {
        return sesId + chIds + QByteArray(1, 0x01);
    }
    else
    {
        return sesId + chIds + QByteArray(1, 0x00);
    }
}

QByteArray toTEXT(const QString &txt)
{
    QByteArray ret = QTextCodec::codecForName(TXT_CODEC)->fromUnicode(txt);

    return ret.mid(2); // removes BOM.
}

QByteArray fixedToTEXT(const QString &txt, int len)
{
    return toTEXT(txt.leftJustified(len, ' ', true));
}

QString fromTEXT(const QByteArray &txt)
{
    return QTextCodec::codecForName(TXT_CODEC)->toUnicode(txt);
}

quint64 rdInt(const QByteArray &bytes)
{
    quint64 ret = 0;

    memcpy(&ret, bytes.data(), static_cast<size_t>(bytes.size()));

    return qFromLittleEndian(ret);
}

bool noCaseMatch(const QString &strA, const QString &strB)
{
    return strA.toLower() == strB.toLower();
}

bool containsNewLine(const QString &str)
{
    bool ret = false;

    for (auto&& chr : str)
    {
        if (chr.category() == QChar::Other_Control)
        {
            ret = true;
        }
    }

    return ret;
}

bool validSubId(const QString &num)
{
    bool ret = false;

    if (isInt(num))
    {
        ret = (num.toInt() >= 0) && (num.toInt() <= 255);
    }

    return ret;
}

bool validUserName(const QString &uName)
{
    bool ret = false;

    if ((uName.size() >= 2) && (uName.size() <= 24))
    {
        ret = !uName.contains(' ') && !containsNewLine(uName);
    }

    return ret;
}

bool validCommonName(const QString &name)
{
    bool ret = false;

    if ((name.size() >= 1) && (name.size() <= 200))
    {
        ret = !name.contains(' ') && !containsNewLine(name);
    }

    return ret;
}

bool validEmailAddr(const QString &email)
{
    bool ret = false;

    QStringList spEmail = email.split('@');

    if ((spEmail.size() == 2) && (email.size() >= 4) && (email.size() <= 64))
    {
        if (!email.contains(' ') && !containsNewLine(email))
        {
            ret = (spEmail[1].split('.').size() > 1);
        }
    }

    return ret;
}

bool validGroupName(const QString &grName)
{
    bool ret = false;

    if ((grName.size() >= 1) && (grName.size() <= 12))
    {
        ret = !grName.contains(' ') && !containsNewLine(grName);
    }

    return ret;
}

bool validCommandName(const QString &name)
{
    bool ret = true;

    if ((name.size() >= 1) && (name.size() <= 64) && !name.contains(' '))
    {
        for (auto&& chr : name)
        {
            if (!chr.isNumber() &&
                !chr.isLetter() &&
                !(chr == '_')   &&
                !(chr == '?'))
            {
                ret = false; break;
            }
        }
    }
    else
    {
        ret = false;
    }

    return ret;
}

bool validDispName(const QString &name)
{
    return (name.size() <= 32) && !containsNewLine(name);
}

bool validChName(const QString &name)
{
    bool ret = false;

    if ((name.size() >= 4) && (name.size() <= 32))
    {
        ret = !name.contains(' ') && !containsNewLine(name);
    }

    return ret;
}

bool validLevel(const QString &num, bool includePub)
{
    bool ret = false;

    if (isInt(num))
    {
        if (includePub)
        {
            ret = (num.toInt() >= 1) && (num.toInt() <= PUBLIC);
        }
        else
        {
            ret = (num.toInt() >= 1) && (num.toInt() <= REGULAR);
        }
    }

    return ret;
}

bool validPassword(const QString &pw)
{
    bool ret = false;

    if ((pw.size() >= 8) && (pw.size() <= 200))
    {
        bool letters = false;
        bool numbers = false;
        bool upper   = false;
        bool lower   = false;
        bool special = false;

        for (int i = 0; i < pw.size(); ++i)
        {
            if (pw[i].isLetter()) {letters = true; break;}
        }

        for (int i = 0; (i < pw.size()) && letters; ++i)
        {
            if (pw[i].isNumber()) {numbers = true; break;}
        }

        for (int i = 0; (i < pw.size()) && numbers; ++i)
        {
            if (pw[i].isUpper()) {upper = true; break;}
        }

        for (int i = 0; (i < pw.size()) && upper; ++i)
        {
            if (pw[i].isLower()) {lower = true; break;}
        }

        for (int i = 0; (i < pw.size()) && lower; ++i)
        {
            if (pw[i].isSymbol() || pw[i].isPunct()) {special = true; break;}
        }

        ret = (letters && numbers && upper && lower && special);
    }

    return ret;
}

bool matchedFsObjTypes(const QString &pathA, const QString &pathB)
{
    QFileInfo infoA(pathA);
    QFileInfo infoB(pathB);

    bool ret = false;

    if      (infoA.isSymLink()) ret = infoB.isSymLink();
    else if (infoA.isFile())    ret = infoB.isFile();
    else                        ret = infoB.isDir();

    return ret;
}

bool matchedVolume(const QString &pathA, const QString &pathB)
{
    QFileInfo infoA(pathA);
    QFileInfo infoB(pathB);

    QStorageInfo storA(infoA.absolutePath());
    QStorageInfo storB(infoB.absolutePath());

    return storA.device() == storB.device();
}

void mkPathForFile(const QString &path)
{
    mkPath(QFileInfo(path).absolutePath());
}

bool userExists(const QString &uName)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_USERNAME);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    return db.rows();
}

bool recoverPWExists(const QString &uName)
{
    Query db;

    db.setType(Query::PULL, TABLE_PW_RECOVERY);
    db.addColumn(COLUMN_USERNAME);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    return db.rows();
}

bool emailExists(const QString &email)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_EMAIL);
    db.addCondition(COLUMN_EMAIL, email);
    db.exec();

    return db.rows();
}

bool groupExists(const QString &grName)
{
    Query db;

    db.setType(Query::PULL, TABLE_GROUPS);
    db.addColumn(COLUMN_GRNAME);
    db.addCondition(COLUMN_GRNAME, grName);
    db.exec();

    return db.rows();
}

bool modExists(const QString &modName)
{
    Query db;

    db.setType(Query::PULL, TABLE_MODULES);
    db.addColumn(COLUMN_MOD_NAME);
    db.addCondition(COLUMN_MOD_NAME, modName);
    db.exec();

    return db.rows();
}

bool rdOnlyFlagExists(const QString &chName, uchar subId, int level)
{
    Query db;

    db.setType(Query::PULL, TABLE_RDONLY_CAST);
    db.addColumn(COLUMN_ACCESS_LEVEL);
    db.addCondition(COLUMN_SUB_CH_ID, subId);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.addCondition(COLUMN_ACCESS_LEVEL, level);
    db.exec();

    return db.rows();
}

bool isLocked(const QString &uName)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_LOCKED);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    return db.getData(COLUMN_LOCKED).toBool();
}

bool commandHasRank(const QString &cmdName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CMD_RANKS);
    db.addColumn(COLUMN_COMMAND);
    db.addCondition(COLUMN_COMMAND, cmdName);
    db.exec();

    return db.rows();
}

bool checkRank(const QString &myGroup, const QString &targetGroup, bool equalAcceptable)
{
    uint myRank     = getRankForGroup(myGroup);
    uint targetRank = getRankForGroup(targetGroup);
    bool ret        = false;

    if (equalAcceptable)
    {
        ret = (myRank <= targetRank);
    }
    else
    {
        ret = (myRank < targetRank);
    }

    return ret;
}

bool maxedInstalledMods()
{
    Query db;

    db.setType(Query::PULL, TABLE_MODULES);
    db.addColumn(COLUMN_MOD_NAME);
    db.exec();

    int    installed = db.rows();
    double max       = (qPow(2, 16) - (MAX_CMDS_PER_MOD * 2)) / MAX_CMDS_PER_MOD;
                       //max commands - (max async commands + max internal commands) / max commands per mod

    return installed >= max;
}

bool isBool(const QString &str)
{
    bool ret    = false;
    int  binary = str.toInt(&ret);

    if (ret)
    {
        ret = (binary == 1) || (binary == 0);
    }

    return ret;
}

bool isInt(const QString &str)
{
    bool ret;

    str.toULongLong(&ret);

    return ret;
}

bool containsChId(const QByteArray &chId, const QByteArray &chIds)
{
    bool ret = false;

    for (int i = 0; i < chIds.size(); i += 9)
    {
        QByteArray id = QByteArray::fromRawData(chIds.data() + i, 9);

        if (id == chId)
        {
            ret = true;

            break;
        }
    }

    return ret;
}

bool matchChs(const QByteArray &chsA, const QByteArray &chsB)
{
    bool ret = false;

    for (int i = 0; i < chsA.size(); i += 9)
    {
        QByteArray id = QByteArray::fromRawData(chsA.data() + i, 9);

        if (containsChId(id, chsB))
        {
            ret = true;

            break;
        }
    }

    return ret;
}

bool containsActiveCh(const QByteArray &chIds)
{
    bool ret = false;

    Query db;

    for (int i = 0; i < chIds.size(); i += 9)
    {
        quint64 chId  = rdInt(QByteArray::fromRawData(chIds.data() + i, 8));
        quint64 subId = rdInt(QByteArray::fromRawData(chIds.data() + (i + 8), 1));

        db.setType(Query::PULL, TABLE_SUB_CHANNELS);
        db.addColumn(COLUMN_CHANNEL_ID);
        db.addCondition(COLUMN_CHANNEL_ID, chId);
        db.addCondition(COLUMN_SUB_CH_ID, subId);
        db.addCondition(COLUMN_ACTIVE_UPDATE, true);
        db.exec();

        if (db.rows())
        {
            ret = true;

            break;
        }
    }

    return ret;
}

bool channelExists(quint64 chId)
{
    Query db;

    db.setType(Query::PULL, TABLE_CHANNELS);
    db.addColumn(COLUMN_CHANNEL_ID);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.exec();

    return db.rows();
}

bool channelExists(const QString &chName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CHANNELS);
    db.addColumn(COLUMN_CHANNEL_NAME);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.exec();

    return db.rows();
}

bool channelSubExists(quint64 chId, uchar subId)
{
    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_SUB_CH_ID);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_SUB_CH_ID, subId);
    db.exec();

    return db.rows();
}

bool channelSubExists(const QString &ch, const QString &sub)
{
    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_SUB_CH_NAME);
    db.addCondition(COLUMN_CHANNEL_NAME, ch);
    db.addCondition(COLUMN_SUB_CH_NAME, sub);
    db.exec();

    return db.rows();
}

bool inviteExists(const QString &uName, const QString &chName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_CHANNEL_NAME);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.addCondition(COLUMN_USERNAME, uName);
    db.addCondition(COLUMN_PENDING_INVITE, true);
    db.exec();

    return db.rows();
}

bool memberExists(const QString &uName, const QString &chName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_CHANNEL_NAME);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    return db.rows();
}

bool globalActiveFlag()
{
    Query db;

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_ACTIVE_UPDATE);
    db.exec();

    return db.getData(COLUMN_ACTIVE_UPDATE).toBool();
}

bool allowMemberDel(const SharedObjs *sharedObjs, const QString &targetUName, const QString &chName)
{
    bool ret = false;

    if (memberExists(targetUName, chName))
    {
        int targetLevel = channelAccessLevel(targetUName, chName);

        if (targetLevel != OWNER)
        {
            if (noCaseMatch(*sharedObjs->userName, targetUName))
            {
                ret = true;
            }
            else if (channelAccessLevel(sharedObjs, chName) < targetLevel)
            {
                ret = true;
            }
        }
    }

    return ret;
}

bool allowLevelChange(const SharedObjs *sharedObjs, int newLevel, const QString &chName)
{
    bool ret     = false;
    int  myLevel = channelAccessLevel(*sharedObjs->userName, chName);

    if ((myLevel == OWNER) && (newLevel == OWNER))
    {
        ret = true;
    }
    else if (newLevel > myLevel)
    {
        ret = true;
    }

    return ret;
}

bool genSubId(const QString &chName, int *newId)
{
    bool ret = false;

    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_SUB_CH_ID);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.exec();

    if (db.rows() < maxSubChannels())
    {
        QList<int> subList;

        for (int i = 0; i < db.rows(); ++i)
        {
            subList.append(db.getData(COLUMN_SUB_CH_ID, i).toInt());
        }

        ret    = true;
        *newId = 0;

        while (subList.contains(*newId)) *newId += 1;
    }

    return ret;
}

bool isChOwner(const QString &uName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_USERNAME);
    db.addCondition(COLUMN_USERNAME, uName);
    db.addCondition(COLUMN_PENDING_INVITE, false);
    db.addCondition(COLUMN_ACCESS_LEVEL, OWNER);
    db.exec();

    return db.rows();
}

int channelAccessLevel(const QString &uName, quint64 chId)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_ACCESS_LEVEL);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_USERNAME, uName);
    db.addCondition(COLUMN_PENDING_INVITE, false);
    db.exec();

    if (db.rows())
    {
        return db.getData(COLUMN_ACCESS_LEVEL).toInt();
    }
    else
    {
        return PUBLIC;
    }
}

int channelAccessLevel(const QString &uName, const QString &chName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_ACCESS_LEVEL);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.addCondition(COLUMN_USERNAME, uName);
    db.addCondition(COLUMN_PENDING_INVITE, false);
    db.exec();

    if (db.rows())
    {
        return db.getData(COLUMN_ACCESS_LEVEL).toInt();
    }
    else
    {
        return PUBLIC;
    }
}

int channelAccessLevel(const SharedObjs *sharedObjs, const QString &chName)
{
    if (*sharedObjs->chOwnerOverride)
    {
        return OWNER;
    }
    else
    {
        return channelAccessLevel(*sharedObjs->userName, chName);
    }
}

int channelAccessLevel(const SharedObjs *sharedObjs, quint64 chId)
{
    if (*sharedObjs->chOwnerOverride)
    {
        return OWNER;
    }
    else
    {
        return channelAccessLevel(*sharedObjs->userName, chId);
    }
}

int lowestAcessLevel(quint64 chId, uchar subId)
{
    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_LOWEST_LEVEL);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_SUB_CH_ID, subId);
    db.exec();

    if (db.rows())
    {
        return db.getData(COLUMN_LOWEST_LEVEL).toInt();
    }
    else
    {
        return 5000;
    }
}

int maxSubChannels()
{
    Query db;

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_MAX_SUB_CH);
    db.exec();

    return db.getData(COLUMN_MAX_SUB_CH).toInt();
}

quint64 getChId(const QString &chName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CHANNELS);
    db.addColumn(COLUMN_CHANNEL_ID);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.exec();

    return db.getData(COLUMN_CHANNEL_ID).toULongLong();
}

uchar getSubId(const QString &chName, const QString &subName)
{
    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_SUB_CH_ID);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.addCondition(COLUMN_SUB_CH_NAME, subName);
    db.exec();

    return static_cast<uchar>(db.getData(COLUMN_SUB_CH_ID).toUInt());
}

int chPos(const QByteArray &id, const QByteArray &chIds)
{
    int ret = -1;

    for (int i = 0; i < chIds.size(); i += 9)
    {
        QByteArray chInList = QByteArray::fromRawData(chIds.data() + i, 9);

        if (chInList == id)
        {
            ret = i;

            break;
        }
    }

    return ret;
}

int countChs(const QByteArray &chIds)
{
    int ret = 0;

    for (int i = 0; i < chIds.size(); i += 9)
    {
        quint64 id = rdInt(QByteArray::fromRawData(chIds.data() + i, 8));

        if (id != 0)
        {
            ret++;
        }
    }

    return ret;
}

int blankChPos(const QByteArray &chIds)
{
    int ret = -1;

    for (int i = 0; i < chIds.size(); i += 9)
    {
        quint64 id = rdInt(QByteArray::fromRawData(chIds.data() + i, 8));

        if (id == 0)
        {
            ret = i;

            break;
        }
    }

    return ret;
}

uint getRankForGroup(const QString &grName)
{
    Query db;

    db.setType(Query::PULL, TABLE_GROUPS);
    db.addColumn(COLUMN_HOST_RANK);
    db.addCondition(COLUMN_GRNAME, grName);
    db.exec();

    return db.getData(COLUMN_HOST_RANK).toUInt();
}

void uniqueAdd(quint16 id, QList<quint16> &list)
{
    if (!list.contains(id))
    {
        list.append(id);
    }
}

void listDir(QList<QPair<QString, QString> > &list, const QString &srcPath, const QString &dstPath)
{
    QDir dir(srcPath);

    dir.setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::DirsFirst);

    QStringList ls = dir.entryList();

    for (int i = 0; i < ls.size(); ++i)
    {
        QPair<QString,QString> srcToDst(srcPath + "/" + ls[i], dstPath + "/" + ls[i]);

        list.append(srcToDst);
    }
}

QString getUserGroup(const QString &uName)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_GRNAME);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    return db.getData(COLUMN_GRNAME).toString();
}

QString getUserNameForEmail(const QString &email)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_USERNAME);
    db.addCondition(COLUMN_EMAIL, email);
    db.exec();

    return db.getData(COLUMN_USERNAME).toString();
}

QString getEmailForUser(const QString &uName)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_EMAIL);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    return db.getData(COLUMN_EMAIL).toString();
}

QString escapeChars(const QString &str, const QChar &escapeChr, const QChar &chr)
{
    QString ret;
    bool    escaped = false;

    if (escapeChr == chr)
    {
        ret = str;
    }
    else
    {
        for (auto&& strChr : str)
        {
            if ((strChr == chr) && !escaped)
            {
                ret.append(escapeChr);
                ret.append(strChr);
            }
            else
            {
                escaped = false;

                if (strChr == escapeChr)
                {
                    escaped = true;
                }

                ret.append(strChr);
            }
        }
    }

    return ret;
}

QList<int> genSequence(int min, int max, int len)
{
    QList<int> ret;

    for (int i = 0; i < len; ++i)
    {
        ret.append(QRandomGenerator::global()->bounded(min, max));
    }

    return ret;
}

QChar genLetter()
{
    // generate random letter from ascii table decimal value 97-122.

    return QChar(static_cast<char>(QRandomGenerator::global()->bounded(97, 122)));
}

QChar genNum()
{
    // generate random number from ascii table decimal value 48-57.

    return QChar(static_cast<char>(QRandomGenerator::global()->bounded(48, 57)));
}

QChar genSpecialChar()
{
    static QString specialChars = "`~!@#$%^&*()-_+=[]{}\\|:;\"'<,>.?/";

    return specialChars[QRandomGenerator::global()->bounded(0, specialChars.size() - 1)];
}

int inRange(int pos, int min, int max)
{
    int ret = pos;

    if (pos < min) ret = min;
    if (pos > max) ret = max;

    return ret;
}

void moveCharLeft(int pos, QString &str)
{
    pos = inRange(pos, 0, str.size() - 1);

    QChar chr = str[pos];

    str.remove(pos, 1);

    if (pos == 0) str.append(chr);
    else          str.insert(pos - 1, chr);
}

void moveCharRight(int pos, QString &str)
{
    pos = inRange(pos, 0, str.size() - 1);

    QChar chr = str[pos];

    str.remove(pos, 1);

    if (pos == str.size() - 1) str.insert(0, chr);
    else                       str.insert(pos + 1, chr);
}

QString genPw()
{
    QString ret;

    QList<int> seq = genSequence(2, 5, 4);

    for (int i = 0; i < seq[0]; ++i)
    {
        ret.append(genLetter());
    }

    for (int i = 0; i < seq[1]; ++i)
    {
        ret.append(genLetter().toUpper());
    }

    for (int i = 0; i < seq[2]; ++i)
    {
        ret.append(genNum());
    }

    for (int i = 0; i < seq[3]; ++i)
    {
        ret.append(genSpecialChar());
    }

    seq = genSequence(0, ret.size() - 1, 10);

    bool toggle = false;

    for (int i : seq)
    {
        if (toggle) moveCharRight(i, ret);
        else        moveCharLeft(i, ret);

        toggle = !toggle;
    }

    return ret;
}

QString modDataPath()
{
    QString ret = qEnvironmentVariable(ENV_MOD_PATH, DEFAULT_MOD_PATH);

    if      (ret.right(1) == '/')  ret.chop(1);
    else if (ret.right(1) == '\\') ret.chop(1);

    ret = expandEnvVariables(ret);

    mkPath(ret);

    return ret;
}

QString pipesPath()
{
    QString ret = qEnvironmentVariable(ENV_PIPE_PATH, DEFAULT_PIPE_PATH);

    if      (ret.right(1) == '/')  ret.chop(1);
    else if (ret.right(1) == '\\') ret.chop(1);

    ret = expandEnvVariables(ret);

    mkPath(ret);

    return ret;
}

void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type);
    Q_UNUSED(context);

    if (!msg.contains("QSslSocket: cannot resolve"))
    {
        Query db;

        db.setType(Query::PUSH, TABLE_DMESG);
        db.addColumn(COLUMN_LOGENTRY, msg);
        db.exec();
    }
}

void mkPath(const QString &path)
{
    if (!QDir().exists(path))
    {
        QDir().mkpath(path);
    }
}

void mkFile(const QString &path)
{
    mkPathForFile(path);

    QFile file(path);

    file.open(QFile::WriteOnly);
    file.write(QByteArray());
    file.close();
}

void wrOpenCh(RWSharedObjs *sharedObjs, const QByteArray &id)
{
    quint64 chId  = rdInt(QByteArray::fromRawData(id.data(), 8));
    quint64 subId = rdInt(QByteArray::fromRawData(id.data() + 8, 1));
    int     rdPos = blankChPos(*sharedObjs->chIds);
    int     wrPos = blankChPos(*sharedObjs->wrAbleChIds);
    int     lvl;

    if (*sharedObjs->chOwnerOverride)
    {
        lvl = OWNER;
    }
    else
    {
        lvl = channelAccessLevel(*sharedObjs->userName, chId);
    }

    sharedObjs->chIds->replace(rdPos, 9, id);

    Query db;

    db.setType(Query::PULL, TABLE_RDONLY_CAST);
    db.addColumn(COLUMN_CHANNEL_ID);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_SUB_CH_ID, subId);
    db.addCondition(COLUMN_ACCESS_LEVEL, lvl);
    db.exec();

    if (db.rows() == 0)
    {
        sharedObjs->wrAbleChIds->replace(wrPos, 9, id);
    }

    if (globalActiveFlag())
    {
        *sharedObjs->activeUpdate = true;
    }
    else
    {
        *sharedObjs->activeUpdate = containsActiveCh(*sharedObjs->chIds);
    }
}

void wrCloseCh(RWSharedObjs *sharedObjs, const QByteArray &id, QByteArray &peerStat)
{
    int        rdPos        = chPos(id, *sharedObjs->chIds);
    int        wrPos        = chPos(id, *sharedObjs->wrAbleChIds);
    bool       needPeerStat = false;
    QByteArray oldChIds     = *sharedObjs->chIds;

    if (*sharedObjs->activeUpdate)
    {
        needPeerStat = true;
    }

    if (rdPos != -1)
    {
        sharedObjs->chIds->replace(rdPos, 9, QByteArray(9, static_cast<char>(0)));

        if (wrPos != -1)
        {
            sharedObjs->wrAbleChIds->replace(wrPos, 9, QByteArray(9, static_cast<char>(0)));
        }

        *sharedObjs->activeUpdate = containsActiveCh(*sharedObjs->chIds);
    }

    if (needPeerStat && (oldChIds != *sharedObjs->chIds))
    {
        QByteArray castHeader = oldChIds + wrInt(PEER_STAT, 8);
        QByteArray data       = toPEER_STAT(*sharedObjs->sessionId, *sharedObjs->chIds, false);

        peerStat = castHeader + data;
    }
    else
    {
        peerStat.clear();
    }
}

void wrCloseCh(RWSharedObjs *sharedObjs, quint64 chId, QByteArray &peerStat)
{
    QByteArray chBa         = wrInt(chId, 64);
    QByteArray oldChIds     = *sharedObjs->chIds;
    bool       needPeerStat = false;

    if (*sharedObjs->activeUpdate)
    {
        needPeerStat = true;
    }

    for (int i = 0; i < sharedObjs->chIds->size(); i += 9)
    {
        if (chBa == QByteArray::fromRawData(sharedObjs->chIds->data() + i, 8))
        {
            sharedObjs->chIds->replace(i, 9, QByteArray(9, static_cast<char>(0)));
        }

        if (chBa == QByteArray::fromRawData(sharedObjs->wrAbleChIds->data() + i, 8))
        {
            sharedObjs->wrAbleChIds->replace(i, 9, QByteArray(9, static_cast<char>(0)));
        }
    }

    *sharedObjs->activeUpdate = containsActiveCh(*sharedObjs->chIds);

    if (needPeerStat && (oldChIds != *sharedObjs->chIds))
    {
        QByteArray castHeader = oldChIds + wrInt(PEER_STAT, 8);
        QByteArray data       = toPEER_STAT(*sharedObjs->sessionId, *sharedObjs->chIds, false);

        peerStat = castHeader + data;
    }
    else
    {
        peerStat.clear();
    }
}

void wrCloseCh(RWSharedObjs *sharedObjs, const QByteArray &id)
{
    QByteArray unused;

    wrCloseCh(sharedObjs, id, unused);
}

void wrCloseCh(RWSharedObjs *sharedObjs, quint64 chId)
{
    QByteArray unused;

    wrCloseCh(sharedObjs, chId, unused);
}

QStringList parseArgs(const QByteArray &data, int maxArgs, int *pos)
{
    QStringList ret;
    QString     arg;
    QString     line      = fromTEXT(data);
    bool        inDQuotes = false;
    bool        inSQuotes = false;
    bool        escaped   = false;

    if (pos != nullptr) *pos = 0;

    for (int i = 0; i < line.size(); ++i)
    {
        if (pos != nullptr) *pos += (TXT_CODEC_BITS / 8);

        if ((line[i] == '\'') && !inDQuotes && !escaped)
        {
            // single quote '

            inSQuotes = !inSQuotes;
        }
        else if ((line[i] == '\"') && !inSQuotes && !escaped)
        {
            // double quote "

            inDQuotes = !inDQuotes;
        }
        else
        {
            escaped = false;

            if (line[i].isSpace() && !inDQuotes && !inSQuotes)
            {
                // space

                if (!arg.isEmpty())
                {
                    ret.append(arg);
                    arg.clear();
                }
            }
            else
            {
                if ((line[i] == '\\') && ((i + 1) < line.size()))
                {
                    if ((line[i + 1] == '\'') || (line[i + 1] == '\"'))
                    {
                        escaped = true;
                    }
                    else
                    {
                        arg.append(line[i]);
                    }
                }
                else
                {
                    arg.append(line[i]);
                }
            }
        }

        if ((ret.size() >= maxArgs) && (maxArgs != -1))
        {
            break;
        }
    }

    if (!arg.isEmpty() && !inDQuotes && !inSQuotes)
    {
        ret.append(arg);
    }

    return ret;
}

QString getParam(const QString &key, const QStringList &args)
{
    // this can be used by command objects to pick out parameters
    // from a command line that are pointed by a name identifier
    // example: -i /etc/some_file, this function should pick out
    // "/etc/some_file" from args if "-i" is passed into key.

    QString ret;

    int pos = args.indexOf(QRegExp(key, Qt::CaseInsensitive));

    if (pos != -1)
    {
        // key found.

        if ((pos + 1) <= (args.size() - 1))
        {
            // check ahead to make sure pos + 1 will not go out
            // of range.

            if (!args[pos + 1].startsWith("-"))
            {
                // the "-" used throughout this application
                // indicates an argument so the above 'if'
                // statement will check to make sure it does
                // not return another argument as a parameter
                // in case a back-to-back "-arg -arg" is
                // present.

                ret = args[pos + 1];
            }
        }
    }

    return ret;
}

bool argExists(const QString &key, const QStringList &args)
{
    return args.contains(key, Qt::CaseInsensitive);
}

SessionCarrier::SessionCarrier(Session *session) : QObject()
{
    sessionObj = session;
}

InternCommand::InternCommand(QObject *parent) : ExternCommand(parent)
{
    rwSharedObjs = nullptr;
}

void InternCommand::setWritableDataShare(RWSharedObjs *sharedObjs)
{
    rwSharedObjs = sharedObjs;

    term();
}

bool InternCommand::loopEnabled()
{
    if (rwSharedObjs == nullptr)
    {
        return false;
    }
    else
    {
        return rwSharedObjs->activeLoopCmds->contains(cmdId);
    }
}

bool InternCommand::moreInputEnabled()
{
    if (rwSharedObjs == nullptr)
    {
        return false;
    }
    else
    {
        return rwSharedObjs->moreInputCmds->contains(cmdId);
    }
}

QString InternCommand::libText()
{
    return INTERN_MOD_NAME;
}

QString InternCommand::parseMd(int offset)
{
    QFile      file(":/docs/intern_commands/" + objectName() + ".md", this);
    QByteArray data;

    if (file.open(QFile::ReadOnly))
    {
        data = file.readAll();
    }

    file.close();

    int targetTags = offset * 6;
    int pos        = -1;
    int len        = 0;

    for (int i = 0, tags = 0; i < data.size(); ++i)
    {
        if (data[i] == '#')
        {
            ++tags;

            if (pos != -1)
            {
                break;
            }
        }
        else if (tags == targetTags)
        {
            len++;

            if (pos == -1)
            {
                pos = i;
            }
        }
    }

    QByteArray ret = data.mid(pos, len).trimmed();

    if (offset == 2)
    {
        ret.chop(3);
        ret.remove(0, 3);
    }

    return ret;
}

QString InternCommand::shortText()
{
    return parseMd(1);
}

QString InternCommand::ioText()
{
    return parseMd(2);
}

QString InternCommand::longText()
{
    return parseMd(3);
}

CommandOutput::CommandOutput(ExternCommand *parent) : QObject(parent)
{
    // this class is used by CmdExecutor to permanently attach a command id to
    // the command object's output data and isolates that id from the object
    // itself. this prevents objects from impersonating another object's output
    // data.

    cmdObj = parent;
}

void CommandOutput::setCmdId(quint16 id)
{
    cmdId = id;
}

void CommandOutput::dataFromCmdObj(const QByteArray &data, uchar typeId)
{
    emit dataOut(cmdId, data, typeId);
}

void CommandOutput::openChIdFromCmdObj(quint64 id, uchar subId)
{
    emit openChById(cmdId, id, subId);
}

void CommandOutput::openChNameFromCmdObj(const QString &ch, const QString &sub)
{
    emit openChByName(cmdId, ch, sub);
}

void CommandOutput::closeChIdFromCmdObj(quint64 id, uchar subId)
{
    emit closeChById(cmdId, id, subId);
}

void CommandOutput::closeChNameFromCmdObj(const QString &ch, const QString &sub)
{
    emit closeChByName(cmdId, ch, sub);
}

void CommandOutput::enableLoopFromCmdObj(bool state)
{
    cmdObj->inLoopMode = state;

    emit enableLoop(cmdId, state);
}

void CommandOutput::enableMoreInputFromCmdObj(bool state)
{
    cmdObj->inMoreInputMode = state;

    emit enableMoreInput(cmdId, state);
}

void CommandOutput::finished()
{
    cmdObj->inMoreInputMode = false;
    cmdObj->inLoopMode      = false;

    emit cmdFinished(cmdId);
}

RWSharedObjs::RWSharedObjs(QObject *parent) : QObject(parent)
{
    commands        = nullptr;
    cmdNames        = nullptr;
    chList          = nullptr;
    chIds           = nullptr;
    wrAbleChIds     = nullptr;
    sessionAddr     = nullptr;
    userName        = nullptr;
    groupName       = nullptr;
    displayName     = nullptr;
    appName         = nullptr;
    clientMajor     = nullptr;
    clientMinor     = nullptr;
    clientPatch     = nullptr;
    sessionId       = nullptr;
    moreInputCmds   = nullptr;
    activeLoopCmds  = nullptr;
    pausedCmds      = nullptr;
    p2pAccepted     = nullptr;
    p2pPending      = nullptr;
    activeUpdate    = nullptr;
    chOwnerOverride = nullptr;
    hostRank        = nullptr;
}

ShellIPC::ShellIPC(const QStringList &args, QObject *parent) : QLocalSocket(parent)
{
    arguments = args;
    pipeName  = pipesPath() + "/" + QString(APP_NAME) + ".TCPServer.Control";

    connect(this, SIGNAL(connected()), this, SLOT(hostConnected()));
    connect(this, SIGNAL(disconnected()), this, SIGNAL(closeInstance()));
    connect(this, SIGNAL(readyRead()), this, SLOT(dataIn()));
}

bool ShellIPC::connectToHost()
{
    connectToServer(pipeName);

    if (!waitForConnected())
    {
        QTextStream(stdout) << "" << endl << "Host instance not running." << endl << endl;
    }

    return state() == QLocalSocket::ConnectedState;
}

void ShellIPC::hostConnected()
{
    write(toTEXT(arguments.join(' ')));
}

void ShellIPC::dataIn()
{
    QTextStream(stdout) << fromTEXT(readAll());

    emit closeInstance();
}
