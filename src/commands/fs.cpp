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

    char flags = 0;

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

DownloadFile::DownloadFile(QObject *parent) : CmdObject(parent) {file = new QFile(this);}
UploadFile::UploadFile(QObject *parent)     : CmdObject(parent) {file = new QFile(this);}
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

void DownloadFile::clear()
{
    file->close();

    buffSize = static_cast<qint64>(qPow(2, MAX_FRAME_BITS) - 1);
    dataSent = 0;
    len      = 0;
    flags    = 0;
}

void DownloadFile::sendChunk()
{
    if (buffSize > len) buffSize = len;

    QByteArray data = file->read(buffSize);

    dataSent += data.size();

    emit procOut(data, GEN_FILE);

    mainTxt(QString::number(dataSent) + "/" + QString::number(len) + "\n");

    if ((dataSent >= len) || file->atEnd())
    {
        clear();
    }
}

void DownloadFile::procIn(const QByteArray &binIn, quint8 dType)
{
    if ((dType == GEN_FILE) && (flags & (MORE_INPUT | LOOPING)))
    {
        sendChunk();
    }
    else if (dType == GEN_FILE)
    {
        clear();

        QStringList args   = parseArgs(binIn, 11);
        QString     path   = getParam("-remote_file", args);
        QString     offStr = getParam("-offset", args);
        QString     lenStr = getParam("-len", args);

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
        else if (!isInt(lenStr))
        {
            errTxt("err: Len '" + lenStr + "' is not valid integer.\n");
        }
        else if (!QFileInfo(path).isFile())
        {
            errTxt("err: The remote file is not a file at all.\n");
        }
        else if (!file->open(QFile::ReadOnly))
        {
            errTxt("err: Unable to open remote file for reading. reason: " + file->errorString() + "\n");
        }
        else
        {
            QString genfileRet = "-from_host";

            len    = lenStr.toLongLong();
            flags |= MORE_INPUT;

            if ((len == 0) || (len > file->size()))
            {
                genfileRet.append(" -len " + QString::number(len));

                len = file->size();
            }

            if (!argExists("-single_step", args))
            {
                flags |= LOOPING;
            }

            file->seek(offStr.toLongLong());

            emit procOut(toTEXT(genfileRet), GEN_FILE);
        }
    }
}

void UploadFile::clear()
{
    file->close();

    force        = false;
    confirm      = false;
    dataReceived = 0;
    len          = 0;
    flags        = 0;
    mode         = nullptr;
}

void UploadFile::wrToFile(const QByteArray &data)
{
    dataReceived += data.size();

    file->write(data);

    mainTxt(QString::number(dataReceived) + "/" + QString::number(len) + "\n");

    if (dataReceived >= len)
    {
        clear();
    }
    else if (flags & SINGLE_STEP_MODE)
    {
        emit procOut(QByteArray(), GEN_FILE);
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
        emit procOut(QByteArray(), GEN_FILE);
    }
    else
    {
        errTxt("err: Unable to open the remote file for writing. reason: " + file->errorString() + "\n");
        clear();
    }
}

void UploadFile::procIn(const QByteArray &binIn, quint8 dType)
{
    if (((dType == GEN_FILE) || (dType == TEXT)) && confirm)
    {
        QString ans = fromTEXT(binIn);

        if (noCaseMatch("y", ans))
        {
            confirm = false;

            run();
        }
        else if (noCaseMatch("n", ans))
        {
            clear();
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
        clear();

        QStringList args   = parseArgs(binIn, 11);
        QString     lenStr = getParam("-len", args);
        QString     offStr = getParam("-offset", args);
        QString     dst    = getParam("-remote_file", args);
        bool        exists = QFileInfo(dst).exists();

        file->setFileName(dst);

        if (argExists("-truncate", args))
        {
            mode = QFile::ReadWrite | QFile::Truncate;
        }
        else
        {
            mode = QFile::ReadWrite;
        }

        if (argExists("-single_step", args))
        {
            flags |= SINGLE_STEP_MODE;
        }

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
            force  = argExists("-force", args);
            len    = lenStr.toLongLong();
            flags |= MORE_INPUT;

            file->seek(offStr.toLongLong());

            emit procOut(toTEXT("-to_host"), GEN_FILE);

            if (exists && !force) ask();
            else                  run();
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
        QString ans = fromTEXT(binIn);

        if (noCaseMatch("y", ans))
        {
            flags &= ~MORE_INPUT;

            run();
        }
        else if (noCaseMatch("n", ans))
        {
            flags &= ~MORE_INPUT;
        }
        else
        {
            ask();
        }
    }
    else if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 3);

        path = getParam("-path", args);

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

void Copy::clear()
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
    bool ret = true;

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
            clear();
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
            clear();
        }
        else if (!src->open(QFile::ReadOnly))
        {
            errTxt("err: Unable to open the source file '" + srcPath + "' for reading. reason: " + src->errorString() + "\n");
            clear();
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
            QPair<QString,QString> srcToDst = queue.takeFirst();

            srcPath   = srcToDst.first;
            dstPath   = srcToDst.second;
            fromQueue = true;

            src->setFileName(srcPath);
            dst->setFileName(dstPath);

            bool exists = QFileInfo(dstPath).exists();

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
            clear();
        }
    }
    else if ((dType == TEXT) && (flags & MORE_INPUT))
    {
        QString ans = fromTEXT(binIn);

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
                clear();
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
        clear();

        QStringList args  = parseArgs(binIn, 5);
        bool        force = argExists("-force", args);

        srcPath    = getParam("-src", args);
        dstPath    = getParam("-dst", args);
        oriSrcPath = srcPath;

        bool dstExists = QFileInfo(dstPath).exists();

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
        errTxt("err: Unable to do move operation. it's likely the command failed to remove the existing destination object or writing to the path is not possible/denied.\n");
        clear();
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
    bool ret = true;

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

    return ret;
}

void MakePath::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        QStringList args = parseArgs(binIn, 2);
        QString     path = getParam("-path", args);

        if (path.isEmpty())
        {
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
        QStringList args      = parseArgs(binIn, 4);
        QString     path      = getParam("-path", args);
        bool        infoFrame = argExists("-info_frame", args);
        bool        noHidden  = argExists("-no_hidden", args);

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
        QStringList args      = parseArgs(binIn, 3);
        QString     path      = getParam("-path", args);
        bool        infoFrame = argExists("-info_frame", args);

        QFileInfo info(path);

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
        QStringList args = parseArgs(binIn, 3);
        QString     path = getParam("-path", args);

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
            QDir::setCurrent(path);

            mainTxt(QDir::currentPath() + "\n");
            async(ASYNC_SET_DIR, PRIV_IPC, toTEXT(path));
        }
    }
}

void Tree::clear()
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
        clear();
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
        QStringList args = parseArgs(binIn, 4);
        QString     path = getParam("-path", args);

        infoFrames = argExists("-info_frame", args);
        noHidden   = argExists("-no_hidden", args);

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
            printList(path);
        }
    }
}
