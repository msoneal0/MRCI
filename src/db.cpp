#include "db.h"

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

void cleanupDbConnection()
{
    QSqlDatabase::removeDatabase(Query::getConnectionName());
}

QString columnType(const QString &column)
{
    QString ret;

    if ((column == COLUMN_IPADDR)       || (column == COLUMN_LOGENTRY)     || (column == COLUMN_USERNAME)     ||
        (column == COLUMN_CHANNEL_NAME) || (column == COLUMN_EMAIL)        || (column == COLUMN_SUB_CH_NAME)  ||
        (column == COLUMN_COMMAND)      || (column == COLUMN_CLIENT_VER)   || (column == COLUMN_DISPLAY_NAME))
    {
        ret = "TEXT COLLATE NOCASE";
    }
    else if ((column == COLUMN_COUNT)          || (column == COLUMN_ACCEPTED)        || (column == COLUMN_LOCKED)          ||
             (column == COLUMN_NEED_PASS)      || (column == COLUMN_NEED_NAME)       || (column == COLUMN_PUB_USERS)       ||
             (column == COLUMN_AUTH_ATTEMPT)   || (column == COLUMN_EMAIL_VERIFIED)  || (column == COLUMN_ENABLE_PW_RESET) ||
             (column == COLUMN_ENABLE_CONFIRM) || (column == COLUMN_RECOVER_ATTEMPT) || (column == COLUMN_ACTIVE_UPDATE)   ||
             (column == COLUMN_PENDING_INVITE))
    {
        ret = "BOOL";
    }
    else if ((column == COLUMN_TEMP_PW_MSG) || (column == COLUMN_ZIPBIN)     || (column == COLUMN_CONFIRM_SUBJECT) ||
             (column == COLUMN_ZIPCOMPRESS) || (column == COLUMN_ZIPEXTRACT) || (column == COLUMN_TEMP_PW_SUBJECT) ||
             (column == COLUMN_MAILERBIN)   || (column == COLUMN_MAIL_SEND)  || (column == COLUMN_CONFIRM_MSG)     ||
             (column == COLUMN_MOD_MAIN))
    {
        ret = "TEXT";
    }
    else if ((column == COLUMN_LOCK_LIMIT)   || (column == COLUMN_PORT)        || (column == COLUMN_BAN_LIMIT)    ||
             (column == COLUMN_HOST_RANK)    || (column == COLUMN_MAXSESSIONS) || (column == COLUMN_LOWEST_LEVEL) ||
             (column == COLUMN_ACCESS_LEVEL) || (column == COLUMN_CHANNEL_ID)  || (column == COLUMN_SUB_CH_ID)    ||
             (column == COLUMN_MAX_SUB_CH)   || (column == COLUMN_INITRANK))
    {
        ret = "INTEGER";
    }
    else if (column == COLUMN_TIME)
    {
        ret = "TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL";
    }
    else if ((column == COLUMN_HASH) || (column == COLUMN_SALT) || (column == COLUMN_USER_ID) ||
             (column == COLUMN_SESSION_ID))
    {
        ret = "BLOB";
    }

    return ret;
}

QByteArray genUniqueHash()
{
    QCryptographicHash hasher(QCryptographicHash::Keccak_256);

    hasher.addData(QDateTime::currentDateTime().toString("yyyyMMddHHmmsszzz").toUtf8());
    hasher.addData(QByteArray::number(QRandomGenerator::global()->generate()));

    return hasher.result();
}

QString sqlDataPath()
{
    return expandEnvVariables(qEnvironmentVariable(ENV_DB_PATH, DEFAULT_DB_FILE));
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

quint32 initHostRank()
{
    Query db;

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_INITRANK);
    db.exec();

    return db.getData(COLUMN_INITRANK).toUInt();
}

QByteArray getSalt(const QByteArray &uId, const QString &table)
{
    Query db;

    db.setType(Query::PULL, table);
    db.addColumn(COLUMN_SALT);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    return db.getData(COLUMN_SALT).toByteArray();
}

QByteArray rootUserId()
{
    Query db;

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_ROOT_USER);
    db.exec();

    QByteArray id = db.getData(COLUMN_ROOT_USER).toByteArray();

    if (id.isEmpty())
    {
        db.setType(Query::PULL, TABLE_USERS);
        db.addColumn(COLUMN_USER_ID);
        db.addCondition(COLUMN_USERNAME, DEFAULT_ROOT_USER);
        db.exec();

        id = db.getData(COLUMN_USER_ID).toByteArray();
    }

    return id;
}

bool createUser(const QString &userName, const QString &email, const QString &dispName, const QString &password)
{
    bool ret = false;

    Query      db;
    QByteArray newUId = genUniqueHash();

    db.setType(Query::PUSH, TABLE_USERS);
    db.addColumn(COLUMN_USERNAME, userName);
    db.addColumn(COLUMN_EMAIL, email);
    db.addColumn(COLUMN_HOST_RANK, initHostRank());
    db.addColumn(COLUMN_DISPLAY_NAME, dispName);
    db.addColumn(COLUMN_EMAIL_VERIFIED, false);
    db.addColumn(COLUMN_NEED_PASS, false);
    db.addColumn(COLUMN_NEED_NAME, false);
    db.addColumn(COLUMN_LOCKED, false);
    db.addColumn(COLUMN_USER_ID, newUId);
    db.addRandBlob(COLUMN_SALT, 128);

    if (db.exec())
    {
        ret = updatePassword(newUId, password, TABLE_USERS);
    }

    return ret;
}

bool createTempPw(const QByteArray &uId, const QString &password)
{
    bool ret = false;

    Query db;

    db.setType(Query::PUSH, TABLE_PW_RECOVERY);
    db.addColumn(COLUMN_USER_ID, uId);
    db.addRandBlob(COLUMN_SALT, 128);

    if (db.exec())
    {
        ret = updatePassword(uId, password, TABLE_PW_RECOVERY);
    }

    return ret;
}

bool updatePassword(const QByteArray &uId, const QString &password, const QString &table, bool requireNewPass)
{
    bool       ret  = false;
    QByteArray salt = getSalt(uId, table);

    if (!salt.isEmpty())
    {
        Query db;

        QCryptographicHash hasher(QCryptographicHash::Keccak_512);

        hasher.addData(QTextCodec::codecForName("UTF-16LE")->fromUnicode(password) + salt);

        db.setType(Query::UPDATE, table);
        db.addColumn(COLUMN_HASH, hasher.result());
        db.addColumn(COLUMN_NEED_PASS, requireNewPass);
        db.addCondition(COLUMN_USER_ID, uId);

        ret = db.exec();
    }

    return ret;
}

bool auth(const QByteArray &uId, const QString &password, const QString &table)
{
    bool       ret  = false;
    QByteArray salt = getSalt(uId, table);

    if (!salt.isEmpty())
    {
        Query db;

        QCryptographicHash hasher(QCryptographicHash::Keccak_512);

        hasher.addData(QTextCodec::codecForName("UTF-16LE")->fromUnicode(password) + salt);

        db.setType(Query::PULL, table);
        db.addColumn(COLUMN_HASH);
        db.addCondition(COLUMN_USER_ID, uId);
        db.exec();

        if (db.rows())
        {
            ret = (hasher.result() == db.getData(COLUMN_HASH).toByteArray());
        }
    }

    return ret;
}

Query::Query(QObject *parent) : QObject(parent)
{
    // this class is an SQL database interface that will be used to store
    // all persistent data for this application. it works by building
    // a query string (qStr + jStr + wStr + limit) to be executed by the
    // QSqlQuery object in the exec() function.

    // QT's QSqlQuery in a multi-threaded app will only work if the
    // thread that originally created the database connection is
    // using it so the next set of logic will actually associate the
    // the current thread's object name to a database connection name.

    // any QThread used throughout this application will create it's
    // own unique object name to make this work.

    createRan      = false;
    restraintAdded = false;
    queryOk        = true;
    rowsAffected   = 0;
    type           = PULL;

    if (!QSqlDatabase::contains(getConnectionName()))
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", getConnectionName());

        db.setConnectOptions("ISC_DPB_LC_CTYPE=UTF16LE");
        db.setDatabaseName(sqlDataPath());

        if (db.open())
        {
            enableForeignKeys(true);
            setTextEncoding("UTF16LE");
        }
        else
        {
            queryOk = false;
            lastErr = db.lastError().databaseText().trimmed();
        }
    }
}

QString Query::errDetail()
{
    QString ret;
    QString errTxt = "none";

    if (!lastErr.isEmpty())
    {
        errTxt = lastErr;
    }

    QTextStream txtOut(&ret);

    txtOut << "     driver error: " << errTxt << Qt::endl;
    txtOut << "     query:        " << qStr << jStr << wStr << limit << Qt::endl;
    txtOut << "     database:     " << sqlDataPath() << Qt::endl;

    auto info = QFileInfo(QFileInfo(sqlDataPath()).path());

    if (!info.isReadable())
    {
        txtOut << "     readable:     database path doesn't have read permissions." << Qt::endl;
    }

    if (!info.isWritable())
    {
        txtOut << "     writable:     database path doesn't have write permissions." << Qt::endl;
    }

    return ret;
}

bool Query::inErrorstate()
{
    return !queryOk;
}

QString Query::getConnectionName()
{
    return QThread::currentThread()->objectName();
}

QSqlDatabase Query::getDatabase()
{
    return QSqlDatabase::database(getConnectionName());
}

void Query::enableForeignKeys(bool state)
{   
    QString str;

    if (state) str = "PRAGMA foreign_keys = ON;";
    else       str = "PRAGMA foreign_keys = OFF;";

    QSqlQuery(str, getDatabase());
}

void Query::setTextEncoding(const QString &encoding)
{
    QSqlQuery("PRAGMA encoding = " + encoding + ";", getDatabase());
}

QStringList Query::tables()
{
    return getDatabase().tables();
}

QStringList Query::columnsInTable(const QString &tbl)
{
    QStringList ret;

    QSqlQuery query("PRAGMA table_info(" + tbl + ");", getDatabase());

    while (query.next())
    {
        ret.append(query.value(1).toString());
    }

    return ret;
}

void Query::setType(QueryType qType, const QString &tbl)
{
    createRan      = false;
    restraintAdded = false;
    rowsAffected   = 0;
    table          = tbl;
    type           = qType;

    qStr.clear();
    wStr.clear();
    jStr.clear();
    limit.clear();
    columnList.clear();
    bindValues.clear();
    lastErr.clear();
    directBind.clear();
    whereBinds.clear();
    columnsAsPassed.clear();
    data.clear();

    QTextStream txt(&qStr);

    switch(type)
    {
    case PUSH:
    {
        txt << "INSERT INTO " << tbl << " (%columns%) VALUES (%binds%)";

        break;
    }
    case PULL: case INNER_JOIN_PULL:
    {
        txt << "SELECT %columns% FROM " << tbl;

        break;
    }
    case UPDATE:
    {
        txt << "UPDATE " << tbl << " SET %columns%";

        break;
    }
    case DEL:
    {
        txt << "DELETE FROM " << tbl;

        break;
    }
    case CREATE_TABLE:
    {
        txt << "CREATE TABLE " << tbl << " (%columns%)";

        break;
    }
    case ALTER_TABLE:
    {
        // alter table for this class is limited to only adding a column.
        // renaming tables is simply not needed at this time.

        txt << "ALTER TABLE " << tbl << " ADD %columns%";

        break;
    }
    }
}

void Query::setQueryLimit(uint value, uint offset)
{
    if ((type == PULL) || (type == INNER_JOIN_PULL))
    {
        limit = " LIMIT " + QString::number(value) + " OFFSET " + QString::number(offset);
    }
}

void Query::increment(const QString &column, double value)
{
    changeValue(column, value, "+");
}

void Query::decrement(const QString &column, double value)
{
    changeValue(column, value, "-");
}

void Query::changeValue(const QString &column, double value, const QString &sign)
{
    if (type == UPDATE)
    {
        columnList.append(column + " = " + column + " " + sign + " " + QString::number(value));
    }

    columnsAsPassed.append(column);
}

void Query::addColumn(const QString &column)
{
    if (((type == PULL) || (type == CREATE_TABLE)) && !columnList.contains(column))
    {
        if (type == CREATE_TABLE) columnList.append(column + " " + columnType(column));
        else                      columnList.append(column);
    }
    else if (type == ALTER_TABLE)
    {
        if (!columnList.isEmpty()) columnList.clear();

        columnList.append(column + " " + columnType(column) + " DEFAULT NULL");
    }

    columnsAsPassed.append(column);
}

void Query::addColumn(const QString &column, const QVariant &dataIn)
{   
    if ((type == PUSH) || (type == UPDATE))
    {
        bindValues.append(dataIn);

        if (type == UPDATE)
        {
            columnList.append(column + " = :" + column);
        }
        else
        {
            columnList.append(column);
        }
    }

    columnsAsPassed.append(column);
}

void Query::addTableColumn(const QString &table, const QString &column)
{
    if ((type == INNER_JOIN_PULL) || (type == PULL))
    {
        columnList.append(table + "." + column);
    }

    columnsAsPassed.append(column);
}

void Query::addRandBlob(const QString &column, int len)
{
    if ((type == PUSH) || (type == UPDATE))
    {
        QString rand = "randomblob(" + QString::number(len) + ")";

        directBind.append(columnList.size());
        bindValues.append(rand);

        if (type == UPDATE)
        {
            columnList.append(column + " = " + rand);
        }
        else
        {
            columnList.append(column);
        }
    }

    columnsAsPassed.append(column);
}

void Query::addJoinCondition(const QString &column, const QString &joinTable, Condition cond)
{
    if (type == INNER_JOIN_PULL)
    {
        QTextStream txt(&jStr);

        if (jStr.contains(joinTable))
        {
            txt << " AND ";
        }
        else
        {
            txt << " INNER JOIN " << joinTable << " ON ";
        }

        if (cond == NOT_EQUAL)
        {
            txt << table << "." << column << " != " << joinTable << "." << column;
        }
        else if (cond == EQUAL)
        {
            txt << table << "." << column << " = " << joinTable << "." << column;
        }
    }
}

void Query::clearConditions()
{
    wStr.clear();
    whereBinds.clear();
}

void Query::addCondition(const QString &column, const QVariant &data, Condition cond, const QString &tbl)
{
    if ((type == PULL) || (type == UPDATE) || (type == DEL) || (type == INNER_JOIN_PULL))
    {
        QTextStream txt(&wStr);

        if (wStr.isEmpty()) txt << " WHERE ";
        else                txt << " AND ";

        if (cond == NOT_EQUAL)
        {
            if (tbl.isEmpty())
            {
                txt << column << " != :where" << whereBinds.size();
            }
            else
            {
                txt << tbl << "." << column << " != :where" << whereBinds.size();
            }

            whereBinds.append(data);
        }
        else if (cond == EQUAL)
        {
            if (tbl.isEmpty())
            {
                txt << column << " = :where" << whereBinds.size();
            }
            else
            {
                txt << tbl << "." << column << " = :where" << whereBinds.size();
            }

            whereBinds.append(data);
        }
        else
        {
            QString escape      = "\\";
            QString escapedData = data.toString();

            escapedData.replace("%", escape + "%");
            escapedData.replace("_", escape + "_");
            escapedData.replace("'", escape + "'");

            if (cond == LIKE_STARTS_WITH)
            {
                escapedData.append("%");
            }
            else if (cond == LIKE_ENDS_WITH)
            {
                escapedData.insert(0, "%");
            }
            else
            {
                escapedData.append("%");
                escapedData.insert(0, "%");
            }

            if (tbl.isEmpty())
            {
                txt << "LIKE('" << escapedData << "', " << column << ", '" << escape << "') > 0";
            }
            else
            {
                txt << "LIKE('" << escapedData << "', " << tbl << "." << column << ", '" << escape << "') > 0";
            }
        }
    }
}

void Query::addUnique(const QString &column)
{
    if ((columnsAsPassed.contains(column)) && (type == CREATE_TABLE))
    {
        columnList.append("UNIQUE (" + column + ")");
    }
}

void Query::setPrimary(const QString &column)
{
    if ((columnsAsPassed.contains(column)) && (type == CREATE_TABLE))
    {
        columnList.append("PRIMARY KEY (" + column + ")");
    }
}

void Query::setPrimaryAsc(const QString &column)
{
    if ((columnsAsPassed.contains(column)) && (type == CREATE_TABLE))
    {
        columnList.append("PRIMARY KEY (" + column + " ASC)");
    }
}

void Query::setPrimaryDesc(const QString &column)
{
    if ((columnsAsPassed.contains(column)) && (type == CREATE_TABLE))
    {
        columnList.append("PRIMARY KEY (" + column + " DESC)");
    }
}

void Query::addForeign(const QString &column, const QString &refTable, const QString &refColum, FKAction onDel, FKAction onUpdate)
{
    if ((columnsAsPassed.contains(column)) && ((type == CREATE_TABLE) || (type == ALTER_TABLE)))
    {
        QString str = "FOREIGN KEY (" + column + ") REFERENCES " + refTable + " (" + refColum + ")";

        switch (onDel)
        {
        case NO_ACTION:   str.append(" ON DELETE NO ACTION");   break;
        case RESTRICT:    str.append(" ON DELETE RESTRICT");    break;
        case SET_NULL:    str.append(" ON DELETE SET NULL");    break;
        case SET_DEFAULT: str.append(" ON DELETE SET DEFAULT"); break;
        case CASCADE:     str.append(" ON DELETE CASCADE");     break;
        }

        switch (onUpdate)
        {
        case NO_ACTION:   str.append(" ON UPDATE NO ACTION");   break;
        case RESTRICT:    str.append(" ON UPDATE RESTRICT");    break;
        case SET_NULL:    str.append(" ON UPDATE SET NULL");    break;
        case SET_DEFAULT: str.append(" ON UPDATE SET DEFAULT"); break;
        case CASCADE:     str.append(" ON UPDATE CASCADE");     break;
        }

        columnList.append(str);
    }
}

void Query::preExec()
{
    QString columnsStr = QStringList(columnList).join(", ");

    qStr.replace("%columns%", columnsStr);
    data.clear();

    if (type == PUSH)
    {
        QString bindsStr;

        for (int i = 0; i < columnList.size(); ++i)
        {
            if (directBind.contains(i))
            {
                bindsStr.append(bindValues[i].toString() + ", ");
            }
            else
            {
                bindsStr.append(":" + columnsAsPassed[i] + ", ");
            }
        }

        bindsStr.chop(2);
        qStr.replace("%binds%", bindsStr);
    }
}

bool Query::createRedirect()
{
    bool ret = false;

    if ((type == CREATE_TABLE) && (tables().contains(table)))
    {
        ret = true;

        QStringList existingColumns = columnsInTable(table);
        QStringList newColumns      = columnsAsPassed;

        for (int i = 0; (i < newColumns.size()) && queryOk; ++i)
        {
            if (!existingColumns.contains(newColumns[i]))
            {
                setType(ALTER_TABLE, table);
                addColumn(newColumns[i]);
                exec();
            }
        }
    }

    return ret;
}

bool Query::createExecuted()
{
    return createRan;
}

bool Query::exec()
{   
    if (!createRedirect())
    {
        preExec();

        QSqlQuery query(getDatabase());

        query.prepare(qStr + jStr + wStr + limit + ";");

        for (int i = 0; i < bindValues.size(); ++i)
        {
            if (!directBind.contains(i))
            {
                query.bindValue(":" + columnsAsPassed[i], bindValues[i]);
            }
        }

        for (int i = 0; i < whereBinds.size(); ++i)
        {
            query.bindValue(":where" + QString::number(i), whereBinds[i]);
        }

        queryOk      = query.exec();
        lastErr      = query.lastError().driverText().trimmed();
        rowsAffected = query.numRowsAffected();

        if (queryOk && query.isSelect())
        {
            while (query.next())
            {
                QList<QVariant> row;

                for (int i = 0; i < columnsAsPassed.size(); ++i)
                {
                    row.append(query.value(i));
                }

                data.append(row);
            }
        }
        else if (queryOk && (type == CREATE_TABLE))
        {
            createRan = true;
        }
    }

    return queryOk;
}

QList<QList<QVariant> > &Query::allData()
{
    return data;
}

QVariant Query::getData(const QString &column, int row)
{
    QVariant ret;

    if ((row < data.size()) && (row >= 0))
    {
        int index = columnsAsPassed.indexOf(column);

        if (index != -1)
        {
            ret = data[row][index];
        }
    }

    return ret;
}

int Query::rows()
{
    int ret = 0;

    if ((type == PULL) || (type == INNER_JOIN_PULL))
    {
        ret = data.size();
    }
    else
    {
        ret = rowsAffected;
    }

    return ret;
}

int Query::columns()
{
    return columnList.size();
}
