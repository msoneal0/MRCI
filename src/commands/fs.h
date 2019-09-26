#ifndef FS_H
#define FS_H

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

class DownloadFile : public InternCommand
{
    Q_OBJECT

private:

    QFile *file;
    qint64 buffSize;
    qint64 len;
    qint64 dataSent;
    bool   ssMode;

    void sendChunk();

public:

    static QString cmdName();

    bool handlesGenfile();
    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit DownloadFile(QObject *parent = nullptr);
};

//-----------------------

class UploadFile : public InternCommand
{
    Q_OBJECT

private:

    QFile::OpenMode mode;
    QFile          *file;
    qint64          len;
    qint64          dataReceived;
    bool            ssMode;
    bool            confirm;
    bool            force;

    void wrToFile(const QByteArray &data);
    void run();
    void ask();

public:

    static QString cmdName();

    bool handlesGenfile();
    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit UploadFile(QObject *parent = nullptr);
};

//-----------------------

class Delete : public InternCommand
{
    Q_OBJECT

private:

    void ask();
    void run();

    QString path;

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit Delete(QObject *parent = nullptr);
};

//------------------------

class Copy : public InternCommand
{
    Q_OBJECT

protected:

    void ask();
    void run();

    QFile                         *src;
    QFile                         *dst;
    bool                           procedAFile;
    bool                           fromQueue;
    bool                           yToAll;
    bool                           nToAll;
    QString                        dstPath;
    QString                        srcPath;
    QString                        oriSrcPath;
    QList<QPair<QString,QString> > queue;

    virtual bool matchingVolumeMatters();
    virtual bool permissionsOk(bool dstExists);
    virtual void runOnMatchingVolume() {}
    virtual void postProcFile() {}
    virtual void preFinish() {}

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit Copy(QObject *parent = nullptr);
};

//------------------------

class Move : public Copy
{
    Q_OBJECT

private:

    bool matchingVolumeMatters();
    bool permissionsOk(bool dstExists);
    void runOnMatchingVolume();
    void postProcFile();
    void preFinish();

public:

    static QString cmdName();

    explicit Move(QObject *parent = nullptr);
};

//-----------------------

class MakePath : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit MakePath(QObject *parent = nullptr);
};

//-----------------------

class ListFiles : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ListFiles(QObject *parent = nullptr);
};

//-----------------------

class FileInfo : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit FileInfo(QObject *parent = nullptr);
};

//-----------------------

class ChangeDir : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ChangeDir(QObject *parent = nullptr);
};

//--------------------------

class Tree : public InternCommand
{
    Q_OBJECT

private:

    QFileInfoList queue;
    bool          infoFrames;
    bool          noHidden;

    void printList(const QString &path);

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit Tree(QObject *parent = nullptr);
};

#endif // FS_H
