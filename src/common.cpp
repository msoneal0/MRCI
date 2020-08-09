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

QString sslCertChain()
{
    return expandEnvVariables(qEnvironmentVariable(ENV_PUB_KEY, DEFAULT_PUB_KEY_NAME));
}

QString sslPrivKey()
{
    return expandEnvVariables(qEnvironmentVariable(ENV_PRIV_KEY, DEFAULT_PRIV_KEY_NAME));
}

QByteArray rdFileContents(const QString &path, QTextStream &msg)
{
    QByteArray ret;

    msg << "Reading file contents: '" << path << "' ";

    QFile file(path);

    if (file.open(QFile::ReadOnly))
    {
        ret = file.readAll();

        if (!ret.isEmpty())
        {
            msg << "[pass]" << Qt::endl;
        }
        else
        {
            msg << "[fail] (0 bytes of data was read from the file, is it empty?)" << Qt::endl;
        }
    }
    else
    {
        msg << "[fail] (" << file.errorString() << ")" << Qt::endl;
    }

    file.close();

    return ret;
}

QString boolStr(bool state)
{
    QString ret;

    if (state) ret = "true";
    else       ret = "false";

    return ret;
}

QString genSerialNumber()
{
    Serial::serialIndex++;

    return QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()) + "-" + QString::number(Serial::serialIndex);
}

void serializeThread(QThread *thr)
{
    thr->setObjectName(genSerialNumber());
}

quint32 toCmdId32(quint16 cmdId, quint16 branchId)
{
    quint32  ret = 0;
    quint32 *dst = &ret;

    memcpy(dst, &cmdId, 2);
    memcpy(dst + 2, &branchId, 2);

    return ret;
}

quint16 toCmdId16(quint32 id)
{
    quint16 ret = 0;

    memcpy(&ret, &id, 2);

    return ret;
}

QByteArray toFixedTEXT(const QString &txt, int len)
{
    return txt.toUtf8().leftJustified(len, 0, true);
}

QByteArray nullTermTEXT(const QString &txt)
{
    return txt.toUtf8() + QByteArray(1, 0x00);
}

bool noCaseMatch(const QString &strA, const QString &strB)
{
    return strA.toLower() == strB.toLower();
}

bool containsNewLine(const QString &str)
{
    auto ret = false;

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
    auto ret = false;

    if (isInt(num))
    {
        ret = (num.toInt() >= 0) && (num.toInt() <= 255);
    }

    return ret;
}

bool validUserName(const QString &uName)
{
    auto ret = false;

    if ((uName.size() >= 2) && (uName.size() <= BLKSIZE_USER_NAME))
    {
        ret = !uName.contains(' ') && !containsNewLine(uName);
    }

    return ret;
}

bool validEmailAddr(const QString &email)
{
    auto ret     = false;
    auto spEmail = email.split('@');

    if ((spEmail.size() == 2) && (email.size() >= 4) && (email.size() <= BLKSIZE_EMAIL_ADDR))
    {
        if (!email.contains(' ') && !containsNewLine(email))
        {
            ret = (spEmail[1].split('.').size() > 1);
        }
    }

    return ret;
}

bool validCommandName(const QString &name)
{
    bool ret = false;

    if ((name.size() >= 1) && (name.size() <= 64))
    {
        ret = !name.contains(' ') && !containsNewLine(name);
    }

    return ret;
}

bool validDispName(const QString &name)
{
    return (name.size() <= BLKSIZE_DISP_NAME) && !containsNewLine(name);
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

bool validModPath(const QString &modPath)
{
    auto ret = !modPath.isEmpty();

    if (ret)
    {
        static const QString forbidden = "|*:\"?<>";

        for (auto&& chr : forbidden)
        {
            if (modPath.contains(chr))
            {
                ret = false;

                break;
            }
        }
    }

    return ret;
}

bool acceptablePw(const QString &pw, const QString &uName, const QString &dispName, const QString &email, QString *errMsg)
{
    auto ret = validPassword(pw);

    if (!ret)
    {
        *errMsg = "err: Invalid password. it must be between 8-200 chars long containing numbers, mixed case letters and special chars.\n";
    }
    else if (ret && !email.isEmpty()) 
    {
        if (pw.contains(email, Qt::CaseInsensitive))
        {
            *errMsg = "err: Invalid password. it contains your email address.\n"; ret = false;
        }
    }
    else if (ret && !uName.isEmpty())
    {
        if (pw.contains(uName, Qt::CaseInsensitive))
        {
            *errMsg = "err: Invalid password. it contains your user name.\n"; ret = false;
        }
    }
    else if (ret && !dispName.isEmpty())
    {
        if (pw.contains(dispName, Qt::CaseInsensitive))
        {
            *errMsg = "err: Invalid password. it contains your display name.\n"; ret = false;
        }
    }

    return ret;
}

bool acceptablePw(const QString &pw, const QByteArray &uId, QString *errMsg)
{
    auto ret = false;

    if (auth(uId, pw, TABLE_USERS))
    {
        *errMsg = "err: Invaild password. you cannot re-use your old password.\n";
    }
    else
    {

        Query db;
        
        db.setType(Query::PULL, TABLE_USERS);
        db.addColumn(COLUMN_EMAIL);
        db.addColumn(COLUMN_USERNAME);
        db.addColumn(COLUMN_DISPLAY_NAME);
        db.addCondition(COLUMN_USER_ID, uId);
        db.exec();

        auto email = db.getData(COLUMN_EMAIL).toString();
        auto uName = db.getData(COLUMN_USERNAME).toString();
        auto dName = db.getData(COLUMN_DISPLAY_NAME).toString();

        ret = acceptablePw(pw, uName, dName, email, errMsg);
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

bool userExists(const QString &uName, QByteArray *uId, QString *email)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_USER_ID);
    db.addColumn(COLUMN_EMAIL);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    if (db.rows() && (uId != nullptr))
    {
        *uId = db.getData(COLUMN_USER_ID).toByteArray();
    }

    if (db.rows() && (email != nullptr))
    {
        *email = db.getData(COLUMN_EMAIL).toString();
    }

    return db.rows();
}

bool recoverPWExists(const QByteArray &uId)
{
    Query db;

    db.setType(Query::PULL, TABLE_PW_RECOVERY);
    db.addColumn(COLUMN_USER_ID);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    return db.rows();
}

bool emailExists(const QString &email, QByteArray *uId)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_USER_ID);
    db.addCondition(COLUMN_EMAIL, email);
    db.exec();

    if (db.rows() && (uId != nullptr))
    {
        *uId = db.getData(COLUMN_USER_ID).toByteArray();
    }

    return db.rows();
}

bool modExists(const QString &modPath)
{
    Query db;

    db.setType(Query::PULL, TABLE_MODULES);
    db.addColumn(COLUMN_MOD_MAIN);
    db.addCondition(COLUMN_MOD_MAIN, modPath);
    db.exec();

    return db.rows();
}

bool rdOnlyFlagExists(quint64 chId, quint8 subId, quint32 level)
{
    Query db;

    db.setType(Query::PULL, TABLE_RDONLY_CAST);
    db.addColumn(COLUMN_ACCESS_LEVEL);
    db.addCondition(COLUMN_SUB_CH_ID, subId);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_ACCESS_LEVEL, level);
    db.exec();

    return db.rows();
}

bool isLocked(const QByteArray &uId)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_LOCKED);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    return db.getData(COLUMN_LOCKED).toBool();
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

bool matchAnyCh(const char *chsA, const char *chsB)
{
    bool ret = false;

    for (int i = 0; i < MAX_OPEN_SUB_CHANNELS; i += BLKSIZE_SUB_CHANNEL)
    {
        if (!isEmptyBlock(chsA + i, BLKSIZE_SUB_CHANNEL))
        {
            if (posOfBlock(chsA + i, chsB, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL) != -1)
            {
                ret = true;

                break;
            }
        }
    }

    return ret;
}

bool fullMatchChs(const char *openChs, const char *comp)
{
    bool ret = true;

    for (int i = 0; i < MAX_OPEN_SUB_CHANNELS; i += BLKSIZE_SUB_CHANNEL)
    {
        if (!isEmptyBlock(comp + i, BLKSIZE_SUB_CHANNEL))
        {
            if (posOfBlock(comp + i, openChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL) == -1)
            {
                ret = false;

                break;
            }
        }
    }

    return ret;
}

void containsActiveCh(const char *subChs, char *actBlock)
{
    if (globalActiveFlag())
    {
        wr8BitToBlock(1, actBlock);
    }
    else
    {
        wr8BitToBlock(0, actBlock);

        Query db;

        for (int i = 0; i < MAX_OPEN_SUB_CHANNELS; i += BLKSIZE_SUB_CHANNEL)
        {
            quint64 chId  = rd64BitFromBlock(subChs + i);
            quint8  subId = rd8BitFromBlock(subChs + (i + 8));

            db.setType(Query::PULL, TABLE_SUB_CHANNELS);
            db.addColumn(COLUMN_CHANNEL_ID);
            db.addCondition(COLUMN_CHANNEL_ID, chId);
            db.addCondition(COLUMN_SUB_CH_ID, subId);
            db.addCondition(COLUMN_ACTIVE_UPDATE, true);
            db.exec();

            if (db.rows())
            {
                wr8BitToBlock(1, actBlock);

                break;
            }
        }
    }
}

void printDatabaseInfo(QTextStream &txt)
{
    auto json   = getDbSettings();
    auto driver = json["driver"].toString();

    txt << "Database Parameters --" << Qt::endl << Qt::endl;
    txt << "Driver: " << driver << Qt::endl;

    if (driver == "QSQLITE")
    {
        txt << "File:   " << sqlDataPath() << Qt::endl;
    }
    else
    {
        txt << "Host:   " << json["host_name"].toString() << Qt::endl;
        txt << "User:   " << json["user_name"].toString() << Qt::endl;
    }

    txt << Qt::endl;
}

QString defaultPw()
{
    Query db;

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_DEFAULT_PASS);
    db.exec();

    return db.getData(COLUMN_DEFAULT_PASS).toString();
}

bool channelExists(const QString &chName, quint64 *chId)
{
    Query db;

    db.setType(Query::PULL, TABLE_CHANNELS);
    db.addColumn(COLUMN_CHANNEL_ID);
    db.addCondition(COLUMN_CHANNEL_NAME, chName);
    db.exec();

    if (db.rows() && (chId != nullptr))
    {
        *chId = db.getData(COLUMN_CHANNEL_ID).toULongLong();
    }

    return db.rows();
}

bool channelSubExists(quint64 chId, const QString &sub, quint8 *subId)
{
    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_SUB_CH_ID);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_SUB_CH_NAME, sub);
    db.exec();

    if (db.rows() && (subId != nullptr))
    {
        *subId = static_cast<quint8>(db.getData(COLUMN_SUB_CH_ID).toUInt());
    }

    return db.rows();
}

bool inviteExists(const QByteArray &uId, quint64 chId)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_CHANNEL_ID);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_USER_ID, uId);
    db.addCondition(COLUMN_PENDING_INVITE, true);
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

bool genSubId(quint64 chId, quint8 *newId)
{
    bool ret = false;

    Query db;

    db.setType(Query::PULL, TABLE_SUB_CHANNELS);
    db.addColumn(COLUMN_SUB_CH_ID);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.exec();

    if (db.rows() < maxSubChannels())
    {
        QList<quint8> subList;

        for (int i = 0; i < db.rows(); ++i)
        {
            subList.append(static_cast<quint8>(db.getData(COLUMN_SUB_CH_ID, i).toUInt()));
        }

        ret    = true;
        *newId = 0;

        while (subList.contains(*newId)) *newId += 1;
    }

    return ret;
}

bool isChOwner(const QByteArray &uId)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_USER_ID);
    db.addCondition(COLUMN_USER_ID, uId);
    db.addCondition(COLUMN_PENDING_INVITE, false);
    db.addCondition(COLUMN_ACCESS_LEVEL, OWNER);
    db.exec();

    return db.rows();
}

int maxSubChannels()
{
    Query db;

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_MAX_SUB_CH);
    db.exec();

    return db.getData(COLUMN_MAX_SUB_CH).toInt();
}

int channelAccessLevel(const QByteArray &uId, const char *override, quint64 chId)
{
    if (rd8BitFromBlock(override) == 1)
    {
        return OWNER;
    }
    else
    {
        return channelAccessLevel(uId, chId);
    }
}

int channelAccessLevel(const QByteArray &uId, quint64 chId)
{
    Query db;

    db.setType(Query::PULL, TABLE_CH_MEMBERS);
    db.addColumn(COLUMN_ACCESS_LEVEL);
    db.addCondition(COLUMN_CHANNEL_ID, chId);
    db.addCondition(COLUMN_USER_ID, uId);
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

QString getUserNameForEmail(const QString &email)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_USERNAME);
    db.addCondition(COLUMN_EMAIL, email);
    db.exec();

    return db.getData(COLUMN_USERNAME).toString();
}

QString getDispName(const QByteArray &uId)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_DISPLAY_NAME);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    return db.getData(COLUMN_DISPLAY_NAME).toString();
}

QString getUserName(const QByteArray &uId)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_USERNAME);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    return db.getData(COLUMN_USERNAME).toString();
}

QString getEmailForUser(const QByteArray &uId)
{
    Query db;

    db.setType(Query::PULL, TABLE_USERS);
    db.addColumn(COLUMN_EMAIL);
    db.addCondition(COLUMN_USER_ID, uId);
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

void mkPath(const QString &path)
{
    if (!QDir().exists(path))
    {
        QDir().mkpath(path);
    }
}

QStringList parseArgs(const QByteArray &data, int maxArgs, int *pos)
{
    QStringList ret;
    QString     arg;

    auto line      = QString::fromUtf8(data);
    auto inDQuotes = false;
    auto inSQuotes = false;
    auto escaped   = false;

    if (pos != nullptr) *pos = 0;

    for (int i = 0; i < line.size(); ++i)
    {
        if (pos != nullptr) *pos += 1;

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

IdleTimer::IdleTimer(QObject *parent) : QTimer(parent)
{
    setSingleShot(true);
}

void IdleTimer::detectWrite(qint64)
{
    start();
}

void IdleTimer::attach(QIODevice *dev, int msec)
{
    setInterval(msec);

    connect(dev, SIGNAL(readyRead()), this, SLOT(start()));
    connect(dev, SIGNAL(bytesWritten(qint64)), this, SLOT(detectWrite(qint64)));
}

ShellIPC::ShellIPC(const QStringList &args, bool supressErr, QObject *parent) : QLocalSocket(parent)
{
    arguments = args;
    holdErrs  = supressErr;

    connect(this, SIGNAL(connected()), this, SLOT(hostConnected()));
    connect(this, SIGNAL(disconnected()), this, SIGNAL(closeInstance()));
    connect(this, SIGNAL(readyRead()), this, SLOT(dataIn()));
}

bool ShellIPC::connectToHost()
{
    auto pipeInfo = QFileInfo(QDir::tempPath() + "/" + HOST_CONTROL_PIPE);

    if (!pipeInfo.exists())
    {
        if (!holdErrs)
        {
            QTextStream(stdout) << "" << Qt::endl << "A host instance is not running." << Qt::endl << Qt::endl;
        }
    }
    else
    {
        connectToServer(HOST_CONTROL_PIPE);

        if (!waitForConnected() && !holdErrs)
        {
            QTextStream(stdout) << "" << Qt::endl << "err: Failed to connect to the host instance control pipe." << Qt::endl;
            QTextStream(stdout) << "err: Reason - " << errorString() << Qt::endl;
        }
    }

    return state() == QLocalSocket::ConnectedState;
}

void ShellIPC::hostConnected()
{
    write(arguments.join(' ').toUtf8());
}

void ShellIPC::dataIn()
{
    QTextStream(stdout) << QString::fromUtf8(readAll());

    emit closeInstance();
}

quint64 Serial::serialIndex = 0;
