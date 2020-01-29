#include "fs.h"

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

QByteArray toFILE_INFO(const QFileInfo &info)
{
    // this function converts some information extracted from a QFileInfo object to
    // a FILE_INFO frame.

    // format: [1byte(flags)][8bytes(createTime)][8bytes(modTime)][8bytes(fileSize)]
    //         [TEXT(fileName)][TEXT(symLinkTarget)]

    //         note: the TEXT strings are 16bit NULL terminated meaning 2 bytes of 0x00
    //               indicate the end of the string.

    //         note: the integer data found in flags, modTime, createTime and fileSize
    //               are formatted in little endian byte order (unsigned).

    auto flags = 0x00;

    if (info.isFile())       flags |= IS_FILE;
    if (info.isDir())        flags |= IS_DIR;
    if (info.isSymLink())    flags |= IS_SYMLNK;
    if (info.isReadable())   flags |= CAN_READ;
    if (info.isWritable())   flags |= CAN_WRITE;
    if (info.isExecutable()) flags |= CAN_EXE;
    if (info.exists())       flags |= EXISTS;

    QByteArray ret;
    QByteArray strTerm(2, 0);

    ret.append(flags);
    ret.append(wrInt(info.birthTime().toMSecsSinceEpoch(), 64));
    ret.append(wrInt(info.lastModified().toMSecsSinceEpoch(), 64));
    ret.append(wrInt(info.size(), 64));
    ret.append(toTEXT(info.fileName()) + strTerm);
    ret.append(toTEXT(info.symLinkTarget() + strTerm));

    return ret;
}

QByteArray toFILE_INFO(const QString &path)
{
    return toFILE_INFO(QFileInfo(path));
}

void mkPathForFile(const QString &path)
{
    mkPath(QFileInfo(path).absolutePath());
}

DownloadFile::DownloadFile(QObject *parent) : CmdObject(parent) {file = new QFile(this); onTerminate();}
UploadFile::UploadFile(QObject *parent)     : CmdObject(parent) {file = new QFile(this); onTerminate();}
Delete::Delete(QObject *parent)             : CmdObject(parent) {}
Copy::Copy(QObject *parent)                 : CmdObject(parent) {src = new QFile(this); dst = new QFile(this);}
Move::Move(QObject *parent)                 : Copy(parent)      {}
MakePath::MakePath(QObject *parent)         : CmdObject(parent) {}
ListFiles::ListFiles(QObject *parent)       : CmdObject(parent) {}
FileInfo::FileInfo(QObject *parent)         : CmdObject(parent) {}
ChangeDir::ChangeDir(QObject *parent)       : CmdObject(parent) {}
Tree::Tree(QObject *parent)                 : CmdObject(parent) {}

QString DownloadFile::cmdName() {return "fs_download";}
QString UploadFile::cmdName()   {return "fs_upload";}
QString Delete::cmdName()       {return "fs_delete";}
QString Copy::cmdName()         {return "fs_copy";}
QString Move::cmdName()         {return "fs_move";}
QString MakePath::cmdName()     {return "fs_mkpath";}
QString ListFiles::cmdName()    {return "fs_list";}
QString FileInfo::cmdName()     {return "fs_info";}
QString ChangeDir::cmdName()    {return "fs_cd";}
QString Tree::cmdName()         {return "fs_tree";}

void DownloadFile::onTerminate()
{
    file->close();

    ssMode    = false;
    paramsSet = false;
    dataSent  = 0;
    len       = 0;
    flags     = 0;
}

void DownloadFile::sendChunk()
{
    auto data = file->read(LOCAL_BUFFSIZE);

    if (file->error() != QFile::NoError)
    {
        retCode = EXECUTION_FAIL;

        errTxt("err: File IO failure: " + file->errorString() + ".\n");
        onTerminate();
    }
    else
    {
        dataSent += data.size();

        emit procOut(data, GEN_FILE);

        mainTxt(QString::number(dataSent) + " / " + QString::number(len) + "\n");

        if ((dataSent >= len) || file->atEnd())
        {
            onTerminate();
        }
    }
}

void DownloadFile::procIn(const QByteArray &binIn, quint8 dType)
{
    if ((dType == GEN_FILE) && binIn.isEmpty() && ssMode && paramsSet)
    {
        sendChunk();
    }
    else if (paramsSet)
    {
        flags |= LOOPING;

        sendChunk();
    }
    else if (dType == GEN_FILE)
    {
        auto args   = parseArgs(binIn, 11);
        auto path   = getParam("-remote_file", args);
        auto offStr = getParam("-offset", args);
        auto lenStr = getParam("-len", args);

        retCode = INVALID_PARAMS;

        file->setFileName(path);

        if (path.isEmpty())
        {   
            errTxt("err: -remote_file not found or is empty.\n");
        }
        else if (!file->exists())
        {
            errTxt("err: File not found.\n");
        }
        else if (!offStr.isEmpty() && !isInt(offStr))
        {
            errTxt("err: Offset '" + offStr + "' is not a valid integer.\n");
        }
        else if (!lenStr.isEmpty() && !isInt(lenStr))
        {
            errTxt("err: Len '" + lenStr + "' is not a valid integer.\n");
        }
        else if (!QFileInfo(path).isFile())
        {
            errTxt("err: The remote file is not a file or does not exists.\n");
        }
        else if (!file->open(QFile::ReadOnly))
        {
            retCode = EXECUTION_FAIL;

            errTxt("err: Unable to open the remote file for reading. reason: " + file->errorString() + "\n");
        }
        else
        {
            len       = lenStr.toLongLong();
            ssMode    = argExists("-single_step", args);
            paramsSet = true;
            retCode   = NO_ERRORS;
            flags    |= MORE_INPUT;

            if ((len == 0) || (len > file->size()))
            {
                len = file->size();
            }

            file->seek(offStr.toLongLong());

            emit procOut(toTEXT("-len " + QString::number(len)), GEN_FILE);
        }
    }
}

void UploadFile::onTerminate()
{
    file->close();

    force        = false;
    confirm      = false;
    ssMode       = false;
    dataReceived = 0;
    len          = 0;
    flags        = 0;
    offs         = 0;
    mode         = nullptr;
}

void UploadFile::wrToFile(const QByteArray &data)
{
    auto written = file->write(data);

    if (written == -1)
    {
        retCode = EXECUTION_FAIL;

        errTxt("err: File IO failure: " + file->errorString() + ".\n");
        onTerminate();
    }
    else
    {
        dataReceived += written;

        mainTxt(QString::number(dataReceived) + " / " + QString::number(len) + "\n");

        if (dataReceived >= len)
        {
            onTerminate();
        }
        else if (ssMode)
        {
            emit procOut(QByteArray(), GEN_FILE);
        }
    }
}

void UploadFile::ask()
{
    confirm = true;

    mainTxt("'" + file->fileName() + "' already exists, do you want to overwrite? (y/n): ");
}

void UploadFile::run()
{
    if (file->open(mode))
    {
        file->seek(offs);

        emit procOut(QByteArray(), GEN_FILE);
    }
    else
    {
        retCode = EXECUTION_FAIL;

        errTxt("err: Unable to open the remote file for writing. reason: " + file->errorString() + "\n");
        onTerminate();
    }
}

void UploadFile::procIn(const QByteArray &binIn, quint8 dType)
{
    if (((dType == GEN_FILE) || (dType == TEXT)) && confirm)
    {
        auto ans = fromTEXT(binIn);

        if (noCaseMatch("y", ans))
        {
            confirm = false;

            run();
        }
        else if (noCaseMatch("n", ans))
        {
            retCode = ABORTED;

            onTerminate();
        }
        else
        {
            ask();
        }
    }
    else if ((dType == GEN_FILE) && (flags & MORE_INPUT))
    {
        wrToFile(binIn);
    }
    else if (dType == GEN_FILE)
    {
        auto args   = parseArgs(binIn, 11);
        auto lenStr = getParam("-len", args);
        auto offStr = getParam("-offset", args);
        auto dst    = getParam("-remote_file", args);

        retCode = INVALID_PARAMS;

        if (dst.isEmpty())
        {
            errTxt("err: The remote file path argument (-remote_file) was not found or is empty.\n");
        }
        else if (lenStr.isEmpty())
        {
            errTxt("err: The data len argument (-len) was not found or is empty.\n");
        }
        else if (!offStr.isEmpty() && !isInt(offStr))
        {
            errTxt("err: Offset '" + offStr + "' is not a valid integer.\n");
        }
        else if (!isInt(lenStr))
        {
            errTxt("err: Len '" + lenStr + "' is not valid integer.\n");
        }
        else
        {
            if (argExists("-truncate", args))
            {
                mode = QFile::ReadWrite | QFile::Truncate;
            }
            else
            {
                mode = QFile::ReadWrite;
            }

            force   = argExists("-force", args);
            ssMode  = argExists("-single_step", args);
            len     = lenStr.toLongLong();
            offs    = offStr.toLongLong();
            retCode = NO_ERRORS;
            flags  |= MORE_INPUT;

            file->setFileName(dst);

            emit procOut(QByteArray(), GEN_FILE);

            if (QFileInfo(dst).exists() && !force)
            {
                ask();
            }
            else
            {
                run();
            }
        }
    }
}

void Delete::ask()
{
    flags |= MORE_INPUT;

    mainTxt("Are you sure you want to delete the object? (y/n): ");
}

void Delete::run()
{
    bool ok;

    QFileInfo info(path);

    if (info.isFile() || info.isSymLink())
    {
        ok = QFile::remove(path);
    }
    else
    {
        ok = QDir(path).removeRecursively();
    }

    if (!ok)
    {
        retCode = EXECUTION_FAIL;
    }

    if (!ok && !info.exists())
    {
        errTxt("err: '" + path + "' already does not exists.\n");
    }
    else if (!ok && info.isWritable())
    {
        errTxt("err: Could not delete '" + path + "' for an unknown reason.\n");
    }
    else if (!ok)
    {
        errTxt("err: Could not delete '" + path + ".' permission denied.\n");
    }
}

void Delete::procIn(const QByteArray &binIn, uchar dType)
{
    if ((flags & MORE_INPUT) && (dType == TEXT))
    {
        auto ans = fromTEXT(binIn);

        if (noCaseMatch("y", ans))
        {
            flags &= ~MORE_INPUT;

            run();
        }
        else if (noCaseMatch("n", ans))
        {
            retCode = ABORTED;
            flags  &= ~MORE_INPUT;
        }
        else
        {
            ask();
        }
    }
    else if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 3);

        path    = getParam("-path", args);
        retCode = INVALID_PARAMS;

        if (path.isEmpty())
        {
            errTxt("err: The object path argument (-path) not found or is empty.\n");
        }
        else if (!QFileInfo(path).exists())
        {
            errTxt("err: Object not found.\n");
        }
        else if (!QFileInfo(path).isWritable())
        {
            errTxt("err: Permission denied.\n");
        }
        else
        {
            if (argExists("-force", args)) run();
            else                           ask();
        }
    }
}

void Copy::onTerminate()
{
    fromQueue   = false;
    procedAFile = false;
    yToAll      = false;
    nToAll      = false;
    flags       = 0;

    src->close();
    dst->close();

    queue.clear();
    srcPath.clear();
    dstPath.clear();
    oriSrcPath.clear();
}

void Copy::ask()
{
    flags |= MORE_INPUT;

    QString opts;

    if (fromQueue) opts = "(y/n/y-all/n-all): ";
    else           opts = "(y/n): ";

    mainTxt("'" + dstPath + "' already exists, do you want to overwrite? " + opts);
}

bool Copy::matchingVolumeMatters()
{
    return false;
}

bool Copy::permissionsOk(bool dstExists)
{
    auto ret = true;

    if (!QFileInfo(srcPath).isReadable())
    {
        errTxt("err: Read permission to '" + srcPath + "' is denied.\n");

        ret = false;
    }
    else if (dstExists && !QFileInfo(dstPath).isWritable())
    {
        errTxt("err: Write permission to '" + dstPath + "' is denied\n");

        ret = false;
    }

    if (ret)
    {
        retCode = NO_ERRORS;
    }
    else
    {
        retCode = EXECUTION_FAIL;
    }

    return ret;
}

void Copy::run()
{
    mkPathForFile(dstPath);

    if (matchingVolumeMatters() && matchedVolume(srcPath, dstPath))
    {
        runOnMatchingVolume();
    }
    else if (QFileInfo(srcPath).isSymLink())
    {
        if (procedAFile) mainTxt("\n");

        mainTxt("mklink: '" + srcPath + "' --> '" + dstPath + "'\n");

        if (QFile::link(QFileInfo(srcPath).symLinkTarget(), dstPath))
        {
            procedAFile = true;

            postProcFile();
        }
        else
        {
            errTxt("err: Unable to re-create the source symlink at the destination path. writing to the path is not possible/denied.\n");

            if (queue.isEmpty())
            {
                onTerminate();
            }
        }
    }
    else if (QFileInfo(srcPath).isDir())
    {
        mkPath(dstPath);
        listDir(queue, srcPath, dstPath);

        flags |= LOOPING;
    }
    else
    {
        if (!dst->open(QFile::WriteOnly | QFile::Truncate))
        {
            errTxt("err: Unable to open the destination file '" + dstPath + "' for writing. reason: " + dst->errorString() + "\n");

            if (queue.isEmpty())
            {
                onTerminate();
            }
        }
        else if (!src->open(QFile::ReadOnly))
        {
            errTxt("err: Unable to open the source file '" + srcPath + "' for reading. reason: " + src->errorString() + "\n");

            if (queue.isEmpty())
            {
                onTerminate();
            }
        }
        else
        {
            if (procedAFile) mainTxt("\n");

            mainTxt("'" + srcPath + "' --> '" + dstPath + "'\n\n");

            flags |= LOOPING;
        }
    }
}

void Copy::procIn(const QByteArray &binIn, uchar dType)
{
    if (flags & LOOPING)
    {
        if (src->isOpen() || dst->isOpen())
        {
            dst->write(src->read(LOCAL_BUFFSIZE));

            mainTxt(QString::number(src->pos()) + "/" + QString::number(src->size()) + "\n");

            if (src->atEnd())
            {
                procedAFile = true;

                src->close();
                dst->close();

                postProcFile();
            }
        }
        else if (!queue.isEmpty())
        {
            auto srcToDst = queue.takeFirst();

            srcPath   = srcToDst.first;
            dstPath   = srcToDst.second;
            fromQueue = true;

            src->setFileName(srcPath);
            dst->setFileName(dstPath);

            auto exists = QFileInfo(dstPath).exists();

            if (exists && !yToAll && !nToAll)
            {
                flags &= ~LOOPING;

                ask();
            }
            else if (!nToAll || !exists)
            {
                run();
            }
        }
        else
        {
            preFinish();
            onTerminate();
        }
    }
    else if ((dType == TEXT) && (flags & MORE_INPUT))
    {
        auto ans = fromTEXT(binIn);

        if (noCaseMatch("y", ans))
        {
            if (fromQueue) flags |= LOOPING;

            flags &= ~MORE_INPUT;

            run();
        }
        else if (noCaseMatch("n", ans))
        {
            flags &= ~MORE_INPUT;

            if (fromQueue)
            {
                flags |= LOOPING;
            }
            else
            {
                onTerminate();
            }
        }
        else if (fromQueue)
        {
            if (noCaseMatch("y-all", ans))
            {
                if (fromQueue) flags |= LOOPING;

                flags &= ~MORE_INPUT;
                yToAll = true;

                run();
            }
            else if (noCaseMatch("n-all", ans))
            {
                if (fromQueue) flags |= LOOPING;

                flags &= ~MORE_INPUT;
                nToAll = true;
            }
            else
            {
                ask();
            }
        }
        else
        {
            ask();
        }
    }
    else if (dType == TEXT)
    {
        onTerminate();

        auto args  = parseArgs(binIn, 5);
        auto force = argExists("-force", args);

        srcPath    = getParam("-src", args);
        dstPath    = getParam("-dst", args);
        oriSrcPath = srcPath;
        retCode    = INVALID_PARAMS;

        auto dstExists = QFileInfo(dstPath).exists();

        src->setFileName(srcPath);
        dst->setFileName(dstPath);

        if (srcPath.isEmpty())
        {
            errTxt("err: The source (-src) argument was not found or is empty.\n");
        }
        else if (dstPath.isEmpty())
        {
            errTxt("err: The destination (-dst) argument was not found or is empty.\n");
        }
        else if (!QFileInfo(srcPath).exists())
        {
            errTxt("err: The source does not exists.\n");
        }
        else if (dstExists && !matchedFsObjTypes(srcPath, dstPath))
        {
            errTxt("err: The existing destination object type (file,dir,symmlink) does not match the source object type.\n");
        }
        else if (permissionsOk(dstExists))
        {
            if      (dstExists && force) run();
            else if (dstExists)          ask();
            else                         run();
        }
    }
}

bool Move::matchingVolumeMatters()
{
    return true;
}

void Move::runOnMatchingVolume()
{
    QFileInfo dstInfo(dstPath);

    if (dstInfo.exists())
    {
        if (dstInfo.isDir()) QDir(dstPath).removeRecursively();
        else                 QFile::remove(dstPath);
    }

    if (!QFile::rename(srcPath, dstPath))
    {
        retCode = EXECUTION_FAIL;

        errTxt("err: Unable to do move operation. it's likely the command failed to remove the existing destination object or writing to the path is not possible/denied.\n");
        onTerminate();
    }
}

void Move::postProcFile()
{
    QFile::remove(srcPath);
}

void Move::preFinish()
{
    if (QFileInfo(oriSrcPath).isDir())
    {
        QDir(oriSrcPath).removeRecursively();
    }
}

bool Move::permissionsOk(bool dstExists)
{
    auto ret = true;

    if (!QFileInfo(srcPath).isReadable() || !QFileInfo(srcPath).isWritable())
    {
        errTxt("err: Read/Write permission(s) to '" + srcPath + "' is denied.\n");

        ret = false;
    }
    else if (dstExists && !QFileInfo(dstPath).isWritable())
    {
        errTxt("err: Write permission to '" + dstPath + "' is denied\n");

        ret = false;
    }

    if (ret)
    {
        retCode = NO_ERRORS;
    }
    else
    {
        retCode = EXECUTION_FAIL;
    }

    return ret;
}

void MakePath::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 2);
        auto path = getParam("-path", args);

        if (path.isEmpty())
        {
            retCode = INVALID_PARAMS;

            errTxt("err: The path argument (-path) was not found or is empty.\n");
        }
        else
        {
            mkPath(path);
        }
    }
}

void ListFiles::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args      = parseArgs(binIn, 4);
        auto path      = getParam("-path", args);
        auto infoFrame = argExists("-info_frame", args);
        auto noHidden  = argExists("-no_hidden", args);

        if (path.isEmpty())
        {
            path = QDir::currentPath();
        }

        QFileInfo pathInfo(path);

        retCode = INVALID_PARAMS;

        if (!pathInfo.exists())
        {
            errTxt("err: '" + path + "' does not exists.\n");
        }
        else if (!pathInfo.isDir())
        {
            errTxt("err: '" + path + "' is not a directory.\n");
        }
        else if (!pathInfo.isReadable())
        {
            errTxt("err: Cannot read '" + path + "' permission denied.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            QDir dir(path);

            if (noHidden)
            {
                dir.setFilter(QDir::AllEntries | QDir::System | QDir::NoDotAndDotDot);
            }
            else
            {
                dir.setFilter(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
            }

            dir.setSorting(QDir::DirsFirst | QDir::Name);

            QFileInfoList list = dir.entryInfoList();

            for (auto&& info : list)
            {
                if (infoFrame)
                {
                    emit procOut(toFILE_INFO(info), FILE_INFO);
                }
                else if (info.isDir())
                {
                    mainTxt(info.fileName() + "/" + "\n");
                }
                else
                {
                    mainTxt(info.fileName() + "\n");
                }
            }
        }
    }
}

void FileInfo::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args      = parseArgs(binIn, 3);
        auto path      = getParam("-path", args);
        auto infoFrame = argExists("-info_frame", args);

        QFileInfo info(path);

        retCode = INVALID_PARAMS;

        if (path.isEmpty())
        {
            errTxt("err: The path argument (-path) was not found or is empty.\n");
        }
        else if (!info.exists())
        {
            errTxt("err: Object not found.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            if (infoFrame)
            {
                emit procOut(toFILE_INFO(info), FILE_INFO);
            }
            else
            {
                QString      txt;
                QTextStream  txtOut(&txt);
                QStorageInfo storInfo(path);

                txtOut << "is_file:   " << boolStr(info.isFile())    << endl;
                txtOut << "is_dir:    " << boolStr(info.isDir())     << endl;
                txtOut << "is_symlnk: " << boolStr(info.isSymLink()) << endl << endl;

                txtOut << "can_read:    " << boolStr(info.isReadable())   << endl;
                txtOut << "can_write:   " << boolStr(info.isWritable())   << endl;
                txtOut << "can_execute: " << boolStr(info.isExecutable()) << endl << endl;

                txtOut << "bytes: " << QString::number(info.size()) << endl << endl;

                txtOut << "device: " << storInfo.device() << endl << endl;

                txtOut << "time_created:  " << info.birthTime().toString("MM/dd/yyyy hh:mm:ss AP t")    << endl;
                txtOut << "last_modified: " << info.lastModified().toString("MM/dd/yyyy hh:mm:ss AP t") << endl;
                txtOut << "last_accessed: " << info.lastRead().toString("MM/dd/yyyy hh:mm:ss AP t")     << endl;

                mainTxt(txt);
            }
        }
    }
}

void ChangeDir::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 3);
        auto path = getParam("-path", args);

        retCode = INVALID_PARAMS;

        if (path.isEmpty())
        {
            mainTxt(QDir::currentPath() + "\n");
        }
        else if (!QFileInfo(path).exists())
        {
            errTxt("err: '" + path + "' does not exists.\n");
        }
        else if (!QFileInfo(path).isDir())
        {
            errTxt("err: '" + path + "' is not a directory.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            QDir::setCurrent(path);

            mainTxt(QDir::currentPath() + "\n");
            async(ASYNC_SET_DIR, PRIV_IPC, toTEXT(path));
        }
    }
}

void Tree::onTerminate()
{
    queue.clear();

    flags      = 0;
    infoFrames = false;
    noHidden   = false;
}

void Tree::printList(const QString &path)
{
    QDir dir(path);

    if (noHidden)
    {
        dir.setFilter(QDir::AllEntries | QDir::System | QDir::NoDotAndDotDot);
    }
    else
    {
        dir.setFilter(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    }

    dir.setSorting(QDir::DirsFirst | QDir::Name);

    QFileInfoList list = dir.entryInfoList();

    for (auto&& info : list)
    {
        if (infoFrames)
        {
            emit procOut(toFILE_INFO(info), FILE_INFO);
        }
        else if (info.isDir())
        {
            mainTxt(info.filePath() + "/" + "\n");
        }
        else
        {
            mainTxt(info.filePath() + "\n");
        }

        if (info.isDir())
        {
            queue.append(info);
        }
    }

    if (queue.isEmpty())
    {
        onTerminate();
    }
    else
    {
        flags |= LOOPING;
    }
}

void Tree::procIn(const QByteArray &binIn, quint8 dType)
{
    if (flags & LOOPING)
    {
        printList(queue.takeFirst().filePath());
    }
    else if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 4);
        auto path = getParam("-path", args);

        infoFrames = argExists("-info_frame", args);
        noHidden   = argExists("-no_hidden", args);
        retCode    = INVALID_PARAMS;

        if (path.isEmpty())
        {
            path = QDir::currentPath();
        }

        QFileInfo pathInfo(path);

        if (!pathInfo.exists())
        {
            errTxt("err: '" + path + "' does not exists.\n");
        }
        else if (!pathInfo.isDir())
        {
            errTxt("err: '" + path + "' is not a directory.\n");
        }
        else if (!pathInfo.isReadable())
        {
            errTxt("err: Cannot read '" + path + "' permission denied.\n");
        }
        else
        {
            retCode = NO_ERRORS;
            
            printList(path);
        }
    }
}
