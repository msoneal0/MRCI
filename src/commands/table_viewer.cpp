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

TableViewer::TableViewer(QObject *parent) : InternCommand(parent)
{
    delOption = false;

    term();
}

void TableViewer::term()
{
    emit enableMoreInput(false);

    del    = false;
    offset = 0;

    cachedArgs.clear();
    db.setType(Query::PULL, "");
}

void TableViewer::setParams(const QString &tbl, const QStringList &colms, bool allowDel)
{
    columns   = colms;
    table     = tbl;
    delOption = allowDel;
}

void TableViewer::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, -1);
        bool        close = false;

        if (delOption && argExists("-delete", args))
        {
            del = true;
        }

        if (moreInputEnabled() && !del)
        {
            if (args.contains("q", Qt::CaseInsensitive))
            {
                close = true;
            }
            else
            {
                offset += MAX_LS_ENTRIES;
                args    = cachedArgs;
            }
        }

        if (close)
        {
            term();
        }
        else if (moreInputEnabled() && del)
        {
            QString ans = fromTEXT(binIn);

            if (noCaseMatch("y", ans))
            {
                db.exec();

                term();
            }
            else if (noCaseMatch("n", ans))
            {
                term();
            }
            else
            {
                mainTxt("continue? (y/n): ");
            }
        }
        else
        {
            if (del)
            {
                db.setType(Query::DEL, table);
            }
            else
            {
                db.setType(Query::PULL, table);
                db.setQueryLimit(MAX_LS_ENTRIES, offset);
            }

            bool noDelParams = true;

            for (int i = 0; i < columns.size(); ++i)
            {
                if (!del) db.addColumn(columns[i]);

                QString param = getParam("-" + columns[i], args);

                if (!param.isEmpty())
                {
                    if (del)
                    {
                        if (noDelParams) mainTxt("\n");

                        mainTxt("delete param: " + columns[i] + " = " + param + "\n");

                        noDelParams = false;
                    }

                    db.addCondition(columns[i], param, Query::LIKE);
                }
            }

            if (del)
            {
                emit enableMoreInput(true);

                if (noDelParams) mainTxt("\nabout to delete ALL entries in this table.\n");

                mainTxt("\ncontinue? (y/n): ");
            }
            else
            {
                db.exec();

                QStringList        separators;
                QList<QStringList> tableData;
                QList<int>         justLens;

                for (int i = 0; i < columns.size(); ++i)
                {
                    justLens.append(0);
                    separators.append("-------");
                }

                for (int i = 0; i < db.rows() + 2; ++i)
                {
                    QStringList columnData;

                    if (i == 0)
                    {
                        columnData = columns;
                    }
                    else if (i == 1)
                    {
                        columnData = separators;
                    }
                    else
                    {
                        for (int j = 0; j < columns.size(); ++j)
                        {
                            if (columnType(columns[j]) == "BLOB")
                            {
                                columnData.append(db.getData(columns[j], i - 2).toByteArray().toHex());
                            }
                            else
                            {
                                columnData.append(db.getData(columns[j], i - 2).toString());
                            }
                        }
                    }

                    for (int k = 0; k < justLens.size(); ++k)
                    {
                        if (justLens[k] < columnData[k].size()) justLens[k] = columnData[k].size();
                    }

                    tableData.append(columnData);
                }

                mainTxt("\n");

                for (auto&& row : tableData)
                {
                    for (int i = 0; i < row.size(); ++i)
                    {
                        mainTxt(row[i].leftJustified(justLens[i] + 2, ' '));
                    }

                    mainTxt("\n");
                }

                if (db.rows() == MAX_LS_ENTRIES)
                {
                    if (!moreInputEnabled())
                    {
                        cachedArgs = args;

                        emit enableMoreInput(true);
                    }

                    mainTxt("\n[enter or any] more, [q] close: ");
                }
                else
                {
                    term();
                }
            }
        }
    }
}
