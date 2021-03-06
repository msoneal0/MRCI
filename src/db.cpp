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

    if ((column == COLUMN_IPADDR)       || (column == COLUMN_LOGENTRY)     || (column == COLUMN_USERNAME)    ||
        (column == COLUMN_CHANNEL_NAME) || (column == COLUMN_EMAIL)        || (column == COLUMN_SUB_CH_NAME) ||
        (column == COLUMN_COMMAND)      || (column == COLUMN_DISPLAY_NAME))
    {
        ret = "TEXT COLLATE NOCASE";
    }
    else if ((column == COLUMN_COUNT)          || (column == COLUMN_ACCEPTED)        || (column == COLUMN_LOCKED)         ||
             (column == COLUMN_NEED_PASS)      || (column == COLUMN_NEED_NAME)       || (column == COLUMN_PENDING_INVITE) ||
             (column == COLUMN_AUTH_ATTEMPT)   || (column == COLUMN_EMAIL_VERIFIED)  || (column == COLUMN_ACTIVE_UPDATE)  ||
             (column == COLUMN_RECOVER_ATTEMPT))
    {
        ret = "BOOL";
    }
    else if (column == COLUMN_MOD_MAIN)
    {
        ret = "TEXT";
    }
    else if ((column == COLUMN_CHANNEL_ID)   || (column == COLUMN_LOWEST_LEVEL) || (column == COLUMN_SUB_CH_ID)    ||
             (column == COLUMN_HOST_RANK)    || (column == COLUMN_ACCESS_LEVEL))
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
    auto ret = pos;

    if (pos < min) ret = min;
    if (pos > max) ret = max;

    return ret;
}

void moveCharLeft(int pos, QString &str)
{
    pos = inRange(pos, 0, str.size() - 1);

    auto chr = str[pos];

    str.remove(pos, 1);

    if (pos == 0) str.append(chr);
    else          str.insert(pos - 1, chr);
}

void moveCharRight(int pos, QString &str)
{
    pos = inRange(pos, 0, str.size() - 1);

    auto chr = str[pos];

    str.remove(pos, 1);

    if (pos == str.size() - 1) str.insert(0, chr);
    else                       str.insert(pos + 1, chr);
}

QString genPw()
{
    QString ret;

    auto seq = genSequence(2, 5, 4);

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

    auto toggle = false;

    for (int i : seq)
    {
        if (toggle) moveCharRight(i, ret);
        else        moveCharLeft(i, ret);

        toggle = !toggle;
    }

    return ret;
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

bool testDbWritable()
{
    Query db;

    db.setType(Query::CREATE_TABLE, "test");
    db.addColumn("test");
    db.exec();

    db.setType(Query::DEL_TABLE, "test");

    return db.exec();
}

bool createUser(const QString &userName, const QString &email, const QString &dispName, const QString &password, int rank, bool requireNewPass)
{
    auto ret    = false;
    auto newUId = genUniqueHash();

    Query db;

    db.setType(Query::PUSH, TABLE_USERS);
    db.addColumn(COLUMN_USERNAME, userName);
    db.addColumn(COLUMN_EMAIL, email);
    db.addColumn(COLUMN_HOST_RANK, rank);
    db.addColumn(COLUMN_DISPLAY_NAME, dispName);
    db.addColumn(COLUMN_EMAIL_VERIFIED, false);
    db.addColumn(COLUMN_NEED_PASS, false);
    db.addColumn(COLUMN_NEED_NAME, false);
    db.addColumn(COLUMN_LOCKED, false);
    db.addColumn(COLUMN_USER_ID, newUId);
    db.addRandBlob(COLUMN_SALT, 128);

    if (db.exec())
    {
        ret = updatePassword(newUId, password, TABLE_USERS, requireNewPass);
    }

    return ret;
}

bool createTempPw(const QByteArray &uId, const QString &password)
{
    auto ret = false;

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
    auto ret  = false;
    auto salt = getSalt(uId, table);

    if (!salt.isEmpty())
    {
        Query db;

        QCryptographicHash hasher(QCryptographicHash::Keccak_512);

        hasher.addData(password.toUtf8() + salt);

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
    auto ret  = false;
    auto salt = getSalt(uId, table);

    if (!salt.isEmpty())
    {
        Query db;

        QCryptographicHash hasher(QCryptographicHash::Keccak_512);

        hasher.addData(password.toUtf8() + salt);

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
        auto confObj = confObject();
        auto driver  = confObj[CONF_DB_DRIVER].toString();
        auto db      = QSqlDatabase::addDatabase(driver, getConnectionName());

        db.setConnectOptions("ISC_DPB_LC_CTYPE=UTF8");

        if (driver == "QSQLITE")
        {
            db.setDatabaseName(confObj[CONF_DB_ADDR].toString());
        }
        else
        {
            db.setDatabaseName(APP_NAME);
            db.setUserName(confObj[CONF_DB_UNAME].toString());
            db.setHostName(confObj[CONF_DB_ADDR].toString());
            db.setPassword(confObj[CONF_DB_PW].toString());
        }

        if (db.open())
        {
            enableForeignKeys(true);
            setTextEncoding("UTF8");

            if (!testDbWritable())
            {
                queryOk = false;

                qCritical("%s", "Write access to the database is denied.");
            }
        }
        else
        {
            queryOk = false;

            qCritical("%s", db.lastError().databaseText().trimmed().toUtf8().constData());
        }
    }
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
    case DEL_TABLE:

        txt << "DROP TABLE " << tbl;

        break;
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
        auto str = "FOREIGN KEY (" + column + ") REFERENCES " + refTable + " (" + refColum + ")";

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
    auto columnsStr = QStringList(columnList).join(", ");

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
    auto ret = false;

    if ((type == CREATE_TABLE) && (tables().contains(table)))
    {
        ret = true;

        auto existingColumns = columnsInTable(table);
        auto newColumns      = columnsAsPassed;

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
        else if (!queryOk)
        {
            auto errobj = query.lastError();

            qCritical() << "Database failure";
            qCritical() << "Query prep string: " + qStr + jStr + wStr + limit + ";";
            qCritical() << "Driver text: " + errobj.driverText();
            qCritical() << "Database text: " + errobj.databaseText();

            switch (errobj.type())
            {
            case QSqlError::NoError:          qCritical() << "Error type: NoError";          break;
            case QSqlError::ConnectionError:  qCritical() << "Error type: ConnectionError";  break;
            case QSqlError::StatementError:   qCritical() << "Error type: StatementError";   break;
            case QSqlError::TransactionError: qCritical() << "Error type: TransactionError"; break;
            case QSqlError::UnknownError:     qCritical() << "Error type: UnknownError";     break;
            }
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
        auto index = columnsAsPassed.indexOf(column);

        if (index != -1)
        {
            ret = data[row][index];
        }
    }

    return ret;
}

int Query::rows()
{
    auto ret = 0;

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
