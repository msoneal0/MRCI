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

class TableViewer : public InternCommand
{
    Q_OBJECT

protected:

    Query       db;
    QString     table;
    QStringList columns;
    QStringList cachedArgs;
    bool        delOption;
    bool        del;
    uint        offset;

public:

    void term();
    void setParams(const QString &tbl, const QStringList &colms, bool allowDel);
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit TableViewer(QObject *parent = nullptr);
};

#endif // TABLE_VIEWER_H
