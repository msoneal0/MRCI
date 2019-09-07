#ifndef MOD_TESTER_H
#define MOD_TESTER_H

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

#include <QPluginLoader>
#include <QtPlugin>
#include <QObject>

#include "command.h"

#define IMPORT_REV 1

// the import revision is a module compatibility version number
// used by the host to determine if it can successfully load and
// run this library or not. as of right now, the host supports rev1
// and up.

#define LIB_VERSION "1.0.0"
#define LIB_NAME    "MRCITestMod"

// the versioning system for the library itself can be completely
// different from the host import revision.

QString libName();

class Loader : public ModCommandLoader
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "MRCI.host.module")
    Q_INTERFACES(ModCommandLoader)

public:

    bool           hostRevOk(quint64 minRev);
    quint64        rev();
    ExternCommand *cmdObj(const QString &name);
    QStringList    cmdList();

    explicit Loader(QObject *parent = nullptr);
};

//-----------------

class ModText : public ExternCommand
{
    Q_OBJECT

public:

    explicit ModText(QObject *parent = nullptr);

    void    procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType);
    QString shortText();
    QString ioText();
    QString longText();
    QString libText();
};

//--------------------

class ModInput : public ExternCommand
{
    Q_OBJECT

public:

    explicit ModInput(QObject *parent = nullptr);

    void    procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType);
    QString shortText();
    QString ioText();
    QString longText();
    QString libText();
};

//-----------------------

class ModLoop : public ExternCommand
{
    Q_OBJECT

private:

    int index;

public:

    explicit ModLoop(QObject *parent = nullptr);

    void    term();
    void    procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType);
    QString shortText();
    QString ioText();
    QString longText();
    QString libText();
};

//--------------------------

class ModInherit : public ExternCommand
{
    Q_OBJECT

public:

    explicit ModInherit(QObject *parent = nullptr);

    void        procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType);
    QString     shortText();
    QString     ioText();
    QString     longText();
    QString     libText();
    QStringList internRequest();
};

#endif // MOD_TESTER_H
