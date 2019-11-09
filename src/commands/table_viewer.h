#ifndef TABLE_VIEWER_H
#define TABLE_VIEWER_H

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

class TableViewer : public CmdObject
{
    Q_OBJECT

protected:

    Query              rdQuery;
    Query              delQuery;
    bool               condAdded;
    bool               delOption;
    bool               delMode;
    quint32            offset;
    QList<QString>     columns;
    QList<QString>     tables;
    QList<int>         blobIndexes;
    QList<QStringList> columnRows;

    QList<QStringList> toStrings(const QList<QList<QVariant> > &data);
    QList<int>         getColumnLens(const QList<QStringList> &data);
    void               addWhereConds(const QStringList &userArgs);
    void               askDelete();
    void               askPage();
    void               dispData();

    virtual void onDel() {}

public:

    void idle();
    void nextPage();
    void addJointColumn(const QString &tbl, const QString &column);
    void addTableColumn(const QString &tbl, const QString &column);
    void setParams(const QString &mainTbl, bool allowDel);
    void procIn(const QByteArray &binIn, quint8 dType);

    explicit TableViewer(QObject *parent = nullptr);
};

#endif // TABLE_VIEWER_H
