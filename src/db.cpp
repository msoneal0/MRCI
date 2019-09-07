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
        (column == COLUMN_GRNAME)       || (column == COLUMN_EMAIL)        || (column == COLUMN_INITGROUP)    ||
        (column == COLUMN_COMMAND)      || (column == COLUMN_CLIENT_VER)   || (column == COLUMN_COMMON_NAME)  ||
        (column == COLUMN_DISPLAY_NAME) || (column == COLUMN_CHANNEL_NAME) || (column == COLUMN_SUB_CH_NAME))
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
             (column == COLUMN_MOD_NAME)    || (column == COLUMN_MOD_MAIN))
    {
        ret = "TEXT";
    }
    else if ((column == COLUMN_LOCK_LIMIT)   || (column == COLUMN_PORT)        || (column == COLUMN_BAN_LIMIT)    ||
             (column == COLUMN_HOST_RANK)    || (column == COLUMN_MAXSESSIONS) || (column == COLUMN_LOWEST_LEVEL) ||
             (column == COLUMN_ACCESS_LEVEL) || (column == COLUMN_CHANNEL_ID)  || (column == COLUMN_SUB_CH_ID)    ||
             (column == COLUMN_MAX_SUB_CH)   || (column == COLUMN_CMD_ID_OFFS))
    {
        ret = "INTEGER";
    }
    else if (column == COLUMN_TIME)
    {
        ret = "TIMESTAMP DEFAULT CURRENT_TIMESTAMP NOT NULL";
    }
    else if ((column == COLUMN_HASH)     || (column == COLUMN_SALT)    || (column == COLUMN_CERT) ||
             (column == COLUMN_PRIV_KEY) || (column == COLUMN_USER_ID) || (column == COLUMN_SESSION_ID))
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
    QString ret = qEnvironmentVariable(ENV_DB_PATH, DEFAULT_DB_PATH);

    ret = expandEnvVariables(ret);

    QFileInfo info(ret);
    QDir      dir(info.path());

    if (!dir.exists()) dir.mkpath(info.path());

    return ret;
}

QString initGroup()
{
    Query db;

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_INITGROUP);
    db.exec();

    return db.getData(COLUMN_INITGROUP).toString();
}

QByteArray getSalt(const QString &userName, const QString &table)
{
    Query db;

    db.setType(Query::PULL, table);
    db.addColumn(COLUMN_SALT);
    db.addColumn(COLUMN_USER_ID);
    db.addCondition(COLUMN_USERNAME, userName);
    db.exec();

    return db.getData(COLUMN_SALT).toByteArray() + db.getData(COLUMN_USER_ID).toByteArray();
}

bool createUser(const QString &userName, const QString &email, const QString &dispName, const QString &password)
{
    bool ret = false;

    Query db;

    db.setType(Query::PUSH, TABLE_USERS);
    db.addColumn(COLUMN_USERNAME, userName);
    db.addColumn(COLUMN_EMAIL, email);
    db.addColumn(COLUMN_GRNAME, initGroup());
    db.addColumn(COLUMN_DISPLAY_NAME, dispName);
    db.addColumn(COLUMN_EMAIL_VERIFIED, false);
    db.addColumn(COLUMN_NEED_PASS, false);
    db.addColumn(COLUMN_NEED_NAME, false);
    db.addColumn(COLUMN_USER_ID, genUniqueHash());
    db.addRandBlob(COLUMN_SALT, 128);

    if (db.exec())
    {
        ret = updatePassword(userName, password, TABLE_USERS);
    }

    return ret;
}

bool createTempPw(const QString &userName, const QString &email, const QString &password)
{
    bool ret = false;

    Query db;

    db.setType(Query::PUSH, TABLE_PW_RECOVERY);
    db.addColumn(COLUMN_USERNAME, userName);
    db.addColumn(COLUMN_EMAIL, email);
    db.addRandBlob(COLUMN_SALT, 128);

    if (db.exec())
    {
        ret = updatePassword(userName, password, TABLE_PW_RECOVERY);
    }

    return ret;
}

bool updatePassword(const QString &userName, const QString &password, const QString &table, bool requireNewPass)
{
    bool       ret  = false;
    QByteArray salt = getSalt(userName, table);

    if (!salt.isEmpty())
    {
        Query db;

        QCryptographicHash hasher(QCryptographicHash::Keccak_512);

        hasher.addData(QTextCodec::codecForName("UTF-16LE")->fromUnicode(password) + salt);

        db.setType(Query::UPDATE, table);
        db.addColumn(COLUMN_HASH, hasher.result());
        db.addColumn(COLUMN_NEED_PASS, requireNewPass);
        db.addCondition(COLUMN_USERNAME, userName);

        ret = db.exec();
    }

    return ret;
}

bool auth(const QString &userName, const QString &password, const QString &table)
{
    bool       ret  = false;
    QByteArray salt = getSalt(userName, table);

    if (!salt.isEmpty())
    {
        Query db;

        QCryptographicHash hasher(QCryptographicHash::Keccak_512);

        hasher.addData(QTextCodec::codecForName("UTF-16LE")->fromUnicode(password) + salt);

        db.setType(Query::PULL, table);
        db.addColumn(COLUMN_HASH);
        db.addCondition(COLUMN_USERNAME, userName);
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
    // a query string (qStr + wStr + limit) to be executed by the
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

    QTextStream txtOut(&ret);

    txtOut << "     driver error: " << lastErr << endl;
    txtOut << "     query:        " << qStr << endl;
    txtOut << "     db path:      " << sqlDataPath() << endl;

    QFileInfo info = QFileInfo(QFileInfo(sqlDataPath()).path());

    if (!info.isReadable())
    {
        txtOut << "     readable:     database path doesn't have read permissions." << endl;
    }

    if (!info.isWritable())
    {
        txtOut << "     writable:     database path doesn't have write permissions." << endl;
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
    case PULL:
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
    if (type == PULL)
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

void Query::addCondition(const QString &column, const QVariant &data, Condition cond)
{
    if ((type == PULL) || (type == UPDATE) || (type == DEL))
    {
        QTextStream txt(&wStr);

        if (wStr.isEmpty()) txt << " WHERE ";
        else                txt << " AND ";

        if (cond == NOT_EQUAL)
        {
            txt << column << " != :where" << whereBinds.size();

            whereBinds.append(data);
        }
        else if (cond == EQUAL)
        {
            txt << column << " = :where" << whereBinds.size();

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

            txt << "LIKE('" << escapedData << "', " << column << ", '" << escape << "') > 0";
        }
    }
}

void Query::addComma(QString &str, bool *toggle)
{
    if (!*toggle)
    {
        str.append(",");

        *toggle = true;
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

        query.prepare(qStr + wStr + limit + ";");

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

    if (type == PULL) ret = data.size();
    else              ret = rowsAffected;

    return ret;
}

int Query::columns()
{
    return columnList.size();
}
