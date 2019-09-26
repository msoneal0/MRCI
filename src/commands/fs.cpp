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

DownloadFile::DownloadFile(QObject *parent) : InternCommand(parent) {file = new QFile(this);}
UploadFile::UploadFile(QObject *parent)     : InternCommand(parent) {file = new QFile(this);}
Delete::Delete(QObject *parent)             : InternCommand(parent) {}
Copy::Copy(QObject *parent)                 : InternCommand(parent) {src = new QFile(this); dst = new QFile(this);}
Move::Move(QObject *parent)                 : Copy(parent)          {}
MakePath::MakePath(QObject *parent)         : InternCommand(parent) {}
ListFiles::ListFiles(QObject *parent)       : InternCommand(parent) {}
FileInfo::FileInfo(QObject *parent)         : InternCommand(parent) {}
ChangeDir::ChangeDir(QObject *parent)       : InternCommand(parent) {}
Tree::Tree(QObject *parent)                 : InternCommand(parent) {}

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

bool DownloadFile::handlesGenfile()
{
    return true;
}

void DownloadFile::term()
{
    file->close();

    buffSize  = static_cast<qint64>(qPow(2, MAX_FRAME_BITS) - 1);
    ssMode    = false;
    dataSent  = 0;
    len       = 0;

    emit enableLoop(false);
    emit enableMoreInput(false);
}

void DownloadFile::sendChunk()
{
    if (buffSize > len) buffSize = len;

    QByteArray data = file->read(buffSize);

    dataSent += data.size();

    emit dataToClient(data, GEN_FILE);

    mainTxt(QString::number(dataSent) + "/" + QString::number(len) + "\n");

    if ((dataSent >= len) || file->atEnd())
    {
        term();
    }
}

void DownloadFile::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

    if ((dType == GEN_FILE) && (moreInputEnabled() || loopEnabled()))
    {
        sendChunk();
    }
    else if (dType == GEN_FILE)
    {
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

            ssMode = argExists("-single_step", args);
            len    = lenStr.toLongLong();

            if ((len == 0) || (len > file->size()))
            {
                genfileRet.append(" -len " + QString::number(len));

                len = file->size();
            }

            file->seek(offStr.toLongLong());

            emit enableMoreInput(true);
            emit enableLoop(!ssMode);
            emit dataToClient(toTEXT(genfileRet), GEN_FILE);
        }
    }
}

bool UploadFile::handlesGenfile()
{
    return true;
}

void UploadFile::term()
{
    file->close();

    force        = false;
    confirm      = false;
    ssMode       = false;
    dataReceived = 0;
    len          = 0;
    mode         = nullptr;

    emit enableLoop(false);
    emit enableMoreInput(false);
}

void UploadFile::wrToFile(const QByteArray &data)
{
    dataReceived += data.size();

    file->write(data);

    mainTxt(QString::number(dataReceived) + "/" + QString::number(len) + "\n");

    if (dataReceived >= len)
    {
        term();
    }
    else if (ssMode)
    {
        emit dataToClient(QByteArray(), GEN_FILE);
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
        emit dataToClient(QByteArray(), GEN_FILE);
    }
    else
    {
        errTxt("err: Unable to open the remote file for writing. reason: " + file->errorString() + "\n");
        term();
    }
}

void UploadFile::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

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
            term();
        }
        else
        {
            ask();
        }
    }
    else if ((dType == GEN_FILE) && moreInputEnabled())
    {
        wrToFile(binIn);
    }
    else if (dType == GEN_FILE)
    {
        QStringList args   = parseArgs(binIn, 11);
        QString     lenStr = getParam("-len", args);
        QString     offStr = getParam("-offset", args);
        QString     dst    = getParam("-remote_file", args);
        bool        exists = QFileInfo(dst).exists();

        file->setFileName(dst);

        if (argExists("-truncate", args)) mode = QFile::ReadWrite | QFile::Truncate;
        else                              mode = QFile::ReadWrite;

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
            ssMode = argExists("-single_step", args);
            force  = argExists("-force", args);
            len    = lenStr.toLongLong();

            file->seek(offStr.toLongLong());

            emit enableMoreInput(true);
            emit dataToClient(toTEXT("-to_host"), GEN_FILE);

            if (exists && !force) ask();
            else                  run();
        }
    }
}

void Delete::term()
{
    emit enableMoreInput(false);

    path.clear();
}

void Delete::ask()
{
    emit enableMoreInput(true);

    mainTxt("Are you sure you want to delete the object? (y/n): ");
}

void Delete::run()
{
    bool ok;

    if (QFileInfo(path).isFile() || QFileInfo(path).isSymLink())
    {
        ok = QFile::remove(path);
    }
    else
    {
        ok = QDir(path).removeRecursively();
    }

    if (!ok) errTxt("err: Could not delete '" + path + "' for an unknown reason.\n");
}

void Delete::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

    if (moreInputEnabled() && (dType == TEXT))
    {
        QString ans = fromTEXT(binIn);

        if (noCaseMatch("y", ans))
        {
            emit enableMoreInput(false);

            run();
        }
        else if (noCaseMatch("n", ans))
        {
            term();
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

void Copy::term()
{
    fromQueue   = false;
    procedAFile = false;
    yToAll      = false;
    nToAll      = false;

    src->close();
    dst->close();

    queue.clear();
    srcPath.clear();
    dstPath.clear();
    oriSrcPath.clear();

    emit enableLoop(false);
    emit enableMoreInput(false);
}

void Copy::ask()
{
    emit enableMoreInput(true);

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
            term();
        }
    }
    else if (QFileInfo(srcPath).isDir())
    {
        mkPath(dstPath);
        listDir(queue, srcPath, dstPath);

        emit enableLoop(true);
    }
    else
    {
        if (!dst->open(QFile::WriteOnly | QFile::Truncate))
        {
            errTxt("err: Unable to open the destination file '" + dstPath + "' for writing. reason: " + dst->errorString() + "\n");
            term();
        }
        else if (!src->open(QFile::ReadOnly))
        {
            errTxt("err: Unable to open the source file '" + srcPath + "' for reading. reason: " + src->errorString() + "\n");
            term();
        }
        else
        {
            if (procedAFile) mainTxt("\n");

            mainTxt("'" + srcPath + "' --> '" + dstPath + "'\n\n");

            emit enableLoop(true);
        }
    }
}

void Copy::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

    if (loopEnabled())
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
                enableLoop(false);

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
            term();
        }
    }
    else if ((dType == TEXT) && moreInputEnabled())
    {
        QString ans = fromTEXT(binIn);

        if (noCaseMatch("y", ans))
        {
            emit enableLoop(fromQueue);
            emit enableMoreInput(false);

            run();
        }
        else if (noCaseMatch("n", ans))
        {
            emit enableLoop(fromQueue);
            emit enableMoreInput(false);

            if (!loopEnabled()) term();
        }
        else if (fromQueue)
        {
            if (noCaseMatch("y-all", ans))
            {
                emit enableLoop(fromQueue);
                emit enableMoreInput(false);

                yToAll = true;

                run();
            }
            else if (noCaseMatch("n-all", ans))
            {
                emit enableLoop(fromQueue);
                emit enableMoreInput(false);

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
            errTxt("err: The source file (-src) argument was not found or is empty.\n");
        }
        else if (dstPath.isEmpty())
        {
            errTxt("err: The destination file (-dst) argument was not found or is empty.\n");
        }
        else if (!QFileInfo(srcPath).exists())
        {
            errTxt("err: The source file does not exists.\n");
        }
        else if (dstExists && !matchedFsObjTypes(srcPath, dstPath))
        {
            errTxt("err: The existing destination object type does not match the source object type.\n");
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
        term();
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

void MakePath::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

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

void ListFiles::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

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
                    emit dataToClient(toFILE_INFO(info), FILE_INFO);
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

void FileInfo::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

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
                emit dataToClient(toFILE_INFO(info), FILE_INFO);
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

void ChangeDir::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

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
        }
    }
}

void Tree::term()
{
    queue.clear();

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
            emit dataToClient(toFILE_INFO(info), FILE_INFO);
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
        term();

        emit enableLoop(false);
    }
    else
    {
        emit enableLoop(true);
    }
}

void Tree::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs)

    if (loopEnabled())
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
