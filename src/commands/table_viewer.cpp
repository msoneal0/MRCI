#include "table_viewer.h"

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

TableViewer::TableViewer(QObject *parent) : CmdObject(parent)
{
    delOption = false;

    idle();
}

void TableViewer::idle()
{
    flags     = 0;
    offset    = 0;
    delMode   = false;
    condAdded = false;
}

void TableViewer::setParams(const QString &mainTbl, bool allowDel)
{
    rdQuery.setType(Query::INNER_JOIN_PULL, mainTbl);
    delQuery.setType(Query::DEL, mainTbl);

    delOption = allowDel;
}

void TableViewer::addTableColumn(const QString &tbl, const QString &column)
{
    if (columnType(column) == "BLOB")
    {
        blobIndexes.append(columns.size());
    }

    if (columnRows.isEmpty())
    {
        columnRows.append(QStringList());
        columnRows.append(QStringList());
    }

    tables.append(tbl);
    columns.append(column);
    rdQuery.addTableColumn(tbl, column);

    columnRows[0].append(column);
    columnRows[1].append("-------");
}

void TableViewer::addJointColumn(const QString &tbl, const QString &column)
{
    rdQuery.addJoinCondition(column, tbl);
}

void TableViewer::nextPage()
{
    rdQuery.setQueryLimit(MAX_LS_ENTRIES, offset);
    rdQuery.exec();

    dispData();

    if (rdQuery.rows() == MAX_LS_ENTRIES)
    {
        offset += MAX_LS_ENTRIES;

        askPage();
    }
    else
    {
        idle();
    }
}

void TableViewer::addWhereConds(const QStringList &userArgs)
{
    if (delOption)
    {
        delMode = argExists("-delete", userArgs);
    }

    rdQuery.clearConditions();
    delQuery.clearConditions();

    for (int i = 0; i < columns.size(); ++i)
    {
        auto value = getParam("-" + columns[i], userArgs);

        if (!value.isEmpty())
        {
            if (delMode)
            {
                mainTxt("delete parameter: " + columns[i] + " ~= " + value + "\n");

                condAdded = true;
            }

            rdQuery.addCondition(columns[i], value, Query::LIKE, tables[i]);
            delQuery.addCondition(columns[i], value, Query::LIKE, tables[i]);
        }
    }

    if (condAdded)
    {
        mainTxt("\n");
    }
}

void TableViewer::askDelete()
{
    if (condAdded)
    {
        promptTxt("This will delete rows from the table according to the parameters above. continue (y/n)?: ");
    }
    else
    {
        promptTxt("This will delete all rows in the table. continue (y/n)?: ");
    }

    flags |= MORE_INPUT;
}

void TableViewer::askPage()
{
    promptTxt("\nnext page (y/n)?: ");

    flags |= MORE_INPUT;
}

QList<QStringList> TableViewer::toStrings(const QList<QList<QVariant> > &data)
{
    QList<QStringList> ret;

    for (auto&& srcRow: data)
    {
        QStringList dstRow;

        for (int i = 0; i < srcRow.size(); ++i)
        {
            if (blobIndexes.contains(i))
            {
                dstRow.append(srcRow[i].toByteArray().toHex());
            }
            else
            {
                dstRow.append(srcRow[i].toString());
            }
        }

        ret.append(dstRow);
    }

    return ret;
}

QList<int> TableViewer::getColumnLens(const QList<QStringList> &data)
{
    QList<int> ret;

    for (int i = 0; i < columns.size(); ++i)
    {
        ret.append(0);
    }

    for (auto&& row: data)
    {
        for (int i = 0; i < row.size(); ++i)
        {
            if (row[i].size() >= ret[i])
            {
                ret[i] = row[i].size() + 4;
            }
        }
    }

    return ret;
}

void TableViewer::dispData()
{
    auto tableRows = toStrings(rdQuery.allData());
    auto lens      = getColumnLens(columnRows + tableRows);

    for (auto&& row: columnRows + tableRows)
    {
        for (int i = 0; i < row.size(); ++i)
        {
            mainTxt(row[i].leftJustified(lens[i], ' '));
        }

        mainTxt("\n");
    }
}

void TableViewer::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        if (flags & MORE_INPUT)
        {
            auto text = QString::fromUtf8(binIn).toLower();

            if (text == "y")
            {
                if (delMode)
                {
                    delQuery.exec();

                    mainTxt(delQuery.errDetail());
                    idle();
                    onDel();
                }
                else
                {
                    nextPage();
                }
            }
            else if (text == "n")
            {
                idle();
            }
            else
            {
                if (delMode)
                {
                    askDelete();
                }
                else
                {
                    askPage();
                }
            }
        }
        else
        {
            addWhereConds(parseArgs(binIn, -1));

            if (delMode)
            {
                askDelete();
            }
            else
            {
                nextPage();
            }
        }
    }
}
