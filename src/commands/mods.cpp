#include "mods.h"

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

ListMods::ListMods(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_MODULES, QStringList() << COLUMN_MOD_NAME << COLUMN_MOD_MAIN << COLUMN_LOCKED << COLUMN_CMD_ID_OFFS, false);
}

UploadMod::UploadMod(QObject *parent) : InternCommand(parent)
{
    dSize    = 0;
    fileBuff = new QTemporaryFile(this);
    proc     = new QProcess(this);

    proc->setProcessChannelMode(QProcess::SeparateChannels);

    connect(proc, SIGNAL(finished(int)), this, SLOT(procFinished(int)));
    connect(proc, SIGNAL(readyReadStandardOutput()), this, SLOT(rdTextFromProc()));
    connect(proc, SIGNAL(readyReadStandardError()), this, SLOT(rdErrFromProc()));
}

DelMod::DelMod(QObject *parent) : InternCommand(parent) {}

QString ListMods::cmdName()  {return "ls_mods";}
QString DelMod::cmdName()    {return "rm_mod";}
QString UploadMod::cmdName() {return "add_mod";}

QString rmFileSuffix(const QString &filePath)
{
    QString suffix = "." + QFileInfo(filePath).suffix();

    return filePath.left(filePath.size() - suffix.size());
}

bool validFileOnlyName(const QString &fileName)
{
    QString forbidden = "/\\:*?\"<>|";
    bool    ret       = true;

    if (fileName.contains(".."))
    {
        ret = false;
    }
    else
    {
        for (auto&& chr : forbidden)
        {
            if (fileName.contains(chr, Qt::CaseInsensitive))
            {
                ret = false;

                break;
            }
        }
    }

    return ret;
}

bool UploadMod::handlesGenfile()
{
    return true;
}

bool UploadMod::libExists(const QString &path)
{
#ifdef Q_OS_WIN

    return QFileInfo::exists(path + ".dll") ||
           QFileInfo::exists(path + ".DLL");
#endif

#ifdef Q_OS_UNIX

    return QFileInfo::exists(path + ".so");

#endif

#ifdef Q_OS_AIX

    return QFileInfo::exists(path + ".a");

#endif

#ifdef Q_OS_MAC

    return QFileInfo::exists(path + ".so")    ||
           QFileInfo::exists(path + ".dylib") ||
           QFileInfo::exists(path + ".bundle");
#endif
}

void UploadMod::clearOnfailure()
{
    QDir(modPath).removeRecursively();

    Query db(this);

    db.setType(Query::DEL, TABLE_MODULES);
    db.addCondition(COLUMN_MOD_NAME, modName);
    db.exec();
}

void UploadMod::procFinished(int exStatus)
{
    if (exStatus != 0)
    {
        errTxt("\nerr: The file operation stopped on error.\n");
        clearOnfailure();
    }
    else if (!libExists(modPath + "/main"))
    {
        errTxt("\nerr: The module's main library file does not exists.\n");
        clearOnfailure();
    }
    else
    {
        modPath = modPath + "/main";

        Query db(this);

        db.setType(Query::UPDATE, TABLE_MODULES);
        db.addColumn(COLUMN_LOCKED, false);
        db.addCondition(COLUMN_MOD_NAME, modName);
        db.exec();

        emit backendDataOut(ASYNC_ENABLE_MOD, toTEXT(modPath), PUB_IPC_WITH_FEEDBACK);

        mainTxt("\nFinished...");
    }

    term();
}

void UploadMod::term()
{
    if (proc->state() == QProcess::Running)
    {
        proc->blockSignals(true);
        proc->kill();
    }

    fileBuff->close();

    modPath.clear();
    modName.clear();

    dSize = 0;

    emit enableMoreInput(false);
}

void UploadMod::rdErrFromProc()
{
    errTxt(proc->readAllStandardError());
}

void UploadMod::rdTextFromProc()
{
    mainTxt(proc->readAllStandardOutput());
}

void UploadMod::procStartError(QProcess::ProcessError err)
{
    if (err == QProcess::FailedToStart)
    {
        errTxt("err: Could not start the host archiver. reason: " + proc->errorString() + "\n");
        clearOnfailure();
        term();
    }
}

void UploadMod::setup()
{
    if (QLibrary::isLibrary(clientFile))
    {
        QString suffix = QFileInfo(clientFile).completeSuffix();
        QString dst    = modPath + "/main." + suffix;

        mainTxt("copy file: " + fileBuff->fileName() + " --> " + dst + "/n");

        if (QFile::copy(fileBuff->fileName(), dst))
        {
            procFinished(0);
        }
        else
        {
            procFinished(1);
        }
    }
    else
    {
        Query db(this);

        db.setType(Query::PULL, TABLE_SERV_SETTINGS);
        db.addColumn(COLUMN_ZIPBIN);
        db.addColumn(COLUMN_ZIPEXTRACT);
        db.exec();

        QString exePath = db.getData(COLUMN_ZIPBIN).toString();
        QString cmdLine = db.getData(COLUMN_ZIPEXTRACT).toString();

        cmdLine.replace(INPUT_DIR_SUB, "'" + fileBuff->fileName() + "'");
        cmdLine.replace(OUTPUT_DIR_SUB, "'" + modPath + "'");

        proc->blockSignals(false);
        proc->setProgram(expandEnvVariables(exePath));
        proc->setArguments(parseArgs(toTEXT(cmdLine), -1));
        proc->start();
    }
}

void UploadMod::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (moreInputEnabled() && (dType == GEN_FILE) && (proc->state() == QProcess::NotRunning))
    {
        if (fileBuff->write(binIn.data(), binIn.size()) == -1)
        {
            errTxt("err: Temp file write failure, unable to continue.\n");
            clearOnfailure();
            term();
        }
        else
        {
            mainTxt(QString::number(fileBuff->pos()) + "/" + QString::number(dSize) + "\n");

            if (fileBuff->size() >= dSize)
            {
                mainTxt("\nUpload complete...setting up.\n\n");
                setup();
            }
        }
    }
    else if (dType == GEN_FILE)
    {
        QStringList args = parseArgs(binIn, -1);
        QString     name = getParam("-name", args);
        QString     len  = getParam("-len", args);
        bool        sOk  = false;

        clientFile = getParam("-client_file", args);
        dSize      = len.toLongLong(&sOk, 10);
        modPath    = modDataPath() + "/" + name;
        modName    = name;

        if (name.isEmpty())
        {
            errTxt("err: Module name (-name) argument not found or is empty.\n");
        }
        else if (len.isEmpty())
        {
            errTxt("err: Data length (-len) argument not found or is empty.\n");
        }
        if (maxedInstalledMods())
        {
            errTxt("err: Host maximum amount of installed modules has been reached.\n");
        }
        else if (!sOk)
        {
            errTxt("err: The given data length is not a valid integer.\n");
        }
        else if (name.size() > 64)
        {
            errTxt("err: The module name cannot be larger than 64 chars long.\n");
        }
        else if (!validFileOnlyName(name))
        {
            errTxt("err: Module names cannot contain the following chars: /\\:*?\"<>| or the updir sequence: '..'\n");
        }
        else if (modExists(name))
        {
            errTxt("err: A module of the same name already exists.\n");
        }
        else if (!fileBuff->open())
        {
            errTxt("err: Could not open a new temp file for reading/writing.\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::PULL, TABLE_MODULES);
            db.addColumn(COLUMN_MOD_NAME);
            db.exec();

            quint16 idOffs = static_cast<quint16>((db.rows() + 2) * MAX_CMDS_PER_MOD) + 1;

            db.setType(Query::PUSH, TABLE_MODULES);
            db.addColumn(COLUMN_MOD_NAME, name);
            db.addColumn(COLUMN_MOD_MAIN, modPath + "/main");
            db.addColumn(COLUMN_CMD_ID_OFFS, idOffs);
            db.addColumn(COLUMN_LOCKED, true);
            db.exec();

            mainTxt("Input hooked...uploading data.\n\n");

            emit enableMoreInput(true);
            emit dataToClient(toTEXT("-to_host"), GEN_FILE);
            emit dataToClient(QByteArray(), GEN_FILE);
        }
    }
}

void DelMod::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 2);
        QString     name = getParam("-name", args);

        if (name.isEmpty())
        {
            errTxt("err: Module name argument (-name) was not found or is empty.\n");
        }
        else if (!validFileOnlyName(name))
        {
            errTxt("err: Module names cannot contain the following chars: /\\:*?\"<>| or the updir sequence: '..'\n");
        }
        else if (!modExists(name))
        {
            errTxt("err: No such module found: '" + name + "'\n");
        }
        else
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_MODULES);
            db.addColumn(COLUMN_LOCKED, true);
            db.addCondition(COLUMN_MOD_NAME, name);
            db.exec();

            QByteArray modPath = toTEXT(modDataPath() + "/" + name + "/main");

            emit backendDataOut(ASYNC_DISABLE_MOD, modPath, PRIV_IPC);
            emit backendDataOut(ASYNC_DISABLE_MOD, modPath, PUB_IPC_WITH_FEEDBACK);
        }
    }
}
