#ifndef MODS_H
#define MODS_H

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
#include "table_viewer.h"

QString rmFileSuffix(const QString &filePath);
bool    validFileOnlyName(const QString &fileName);

class UploadMod : public InternCommand
{
    Q_OBJECT

private:

    QProcess       *proc;
    QString         clientFile;
    QString         modName;
    QString         modPath;
    QTemporaryFile *fileBuff;
    qint64          dSize;

    void clearOnfailure();
    void setup();
    bool libExists(const QString &path);

private slots:

    void rdTextFromProc();
    void rdErrFromProc();
    void procFinished(int exStatus);
    void procStartError(QProcess::ProcessError err);

public:

    static QString cmdName();

    bool handlesGenfile();
    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit UploadMod(QObject *parent = nullptr);
};

//-------------------------------

class DelMod : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit DelMod(QObject *parent = nullptr);
};

//-------------------------------

class ListMods : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit ListMods(QObject *parent = nullptr);
};

#endif // MODS_H
