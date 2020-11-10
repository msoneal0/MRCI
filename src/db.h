#ifndef DB_H
#define DB_H

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

#include "common.h"

QString     genPw();
QList<int>  genSequence(int min, int max, int len);
QChar       genLetter();
QChar       genNum();
QChar       genSpecialChar();
int         inRange(int pos, int min, int max);
QString     columnType(const QString &column);
QByteArray  getSalt(const QByteArray &uId, const QString &table);
QByteArray  genUniqueHash();
bool        createUser(const QString &userName, const QString &email, const QString &dispName, const QString &password, int rank, bool requireNewPass = false);
bool        createTempPw(const QByteArray &uId, const QString &password);
bool        updatePassword(const QByteArray &uId, const QString &password, const QString &table, bool requireNewPass = false);
bool        auth(const QByteArray &uId, const QString &password, const QString &table);
bool        testDbWritable();
void        cleanupDbConnection();
void        saveDbSettings(const QJsonObject &obj);
void        moveCharLeft(int pos, QString &str);
void        moveCharRight(int pos, QString &str);

class Query : public QObject
{
    Q_OBJECT

public:

    enum QueryType
    {
        UPDATE,
        PUSH,
        PULL,
        DEL,
        INNER_JOIN_PULL,
        CREATE_TABLE,
        ALTER_TABLE,
        DEL_TABLE
    };

    enum Condition
    {
        EQUAL,
        NOT_EQUAL,
        LIKE,
        LIKE_STARTS_WITH,
        LIKE_ENDS_WITH
    };

    enum FKAction
    {
        NO_ACTION,
        RESTRICT,
        SET_NULL,
        SET_DEFAULT,
        CASCADE
    };

    static QString      getConnectionName();
    static QSqlDatabase getDatabase();

    explicit Query(QObject *parent = nullptr);

    void                     addRandBlob(const QString &column, int len);
    void                     addCondition(const QString &column, const QVariant &data, Condition cond = EQUAL, const QString &tbl = QString());
    void                     addJoinCondition(const QString &column, const QString &joinTable, Condition cond = EQUAL);
    void                     addUnique(const QString &column);
    void                     setPrimary(const QString &column);
    void                     setPrimaryAsc(const QString &column);
    void                     setPrimaryDesc(const QString &column);
    void                     setQueryLimit(uint value, uint offset = 0);
    void                     increment(const QString &column, double value);
    void                     decrement(const QString &column, double value);
    void                     addForeign(const QString &column, const QString &refTable, const QString &refColum, FKAction onDel = RESTRICT, FKAction onUpdate = RESTRICT);
    void                     addColumn(const QString &column);
    void                     addColumn(const QString &column, const QVariant &dataIn);
    void                     addTableColumn(const QString &table, const QString &column);
    void                     setType(QueryType qType, const QString &tbl);
    void                     enableForeignKeys(bool state);
    void                     setTextEncoding(const QString &encoding);
    void                     clearConditions();
    int                      rows();
    int                      columns();
    bool                     exec();
    bool                     createExecuted();
    bool                     inErrorstate();
    QStringList              tables();
    QStringList              columnsInTable(const QString &tbl);
    QVariant                 getData(const QString &column, int row = 0);
    QString                  errDetail();
    QString                  errString();
    QList<QList<QVariant> > &allData();

private:

    bool                    createRan;
    bool                    restraintAdded;
    bool                    queryOk;
    int                     rowsAffected;
    QString                 table;
    QString                 lastErr;
    QString                 limit;
    QString                 qStr;
    QString                 wStr;
    QString                 jStr;
    QueryType               type;
    QList<int>              directBind;
    QStringList             columnsAsPassed;
    QList<QString>          columnList;
    QList<QVariant>         bindValues;
    QList<QVariant>         whereBinds;
    QList<QList<QVariant> > data;

    bool createRedirect();
    void postUpdate();
    void preExec();
    void changeValue(const QString &column, double value, const QString &sign);
};

#endif // DB_H
