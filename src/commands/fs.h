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
#include "../cmd_object.h"

QByteArray toFILE_INFO(const QString &path);
QByteArray toFILE_INFO(const QFileInfo &info);
void       mkPathForFile(const QString &path);

class DownloadFile : public CmdObject
{
    Q_OBJECT

private:

    QFile *file;
    bool   ssMode;
    bool   paramsSet;

    void sendChunk();
    void onTerminate();

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit DownloadFile(QObject *parent = nullptr);
};

//-----------------------

class UploadFile : public CmdObject
{
    Q_OBJECT

private:

    QFile::OpenMode mode;
    QFile          *file;
    qint64          offs;
    bool            ssMode;
    bool            confirm;
    bool            force;

    void wrToFile(const QByteArray &data);
    void onTerminate();
    void run();
    void ask();

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit UploadFile(QObject *parent = nullptr);
};

//-----------------------

class Delete : public CmdObject
{
    Q_OBJECT

private:

    void ask();
    void run();

    QString path;

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, uchar dType);

    explicit Delete(QObject *parent = nullptr);
};

//------------------------

class Copy : public CmdObject
{
    Q_OBJECT

protected:

    void ask();
    void run();
    void onTerminate();

    QFile                         *src;
    QFile                         *dst;
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

    void procIn(const QByteArray &binIn, quint8 dType);

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

class MakePath : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit MakePath(QObject *parent = nullptr);
};

//-----------------------

class ListFiles : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ListFiles(QObject *parent = nullptr);
};

//-----------------------

class FileInfo : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit FileInfo(QObject *parent = nullptr);
};

//-----------------------

class ChangeDir : public CmdObject
{
    Q_OBJECT

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit ChangeDir(QObject *parent = nullptr);
};

//--------------------------

class Tree : public CmdObject
{
    Q_OBJECT

private:

    QFileInfoList queue;
    bool          infoFrames;
    bool          noHidden;

    void printList(const QString &path);
    void onTerminate();

public:

    static QString cmdName();

    void procIn(const QByteArray &binIn, quint8 dType);

    explicit Tree(QObject *parent = nullptr);
};

#endif // FS_H
