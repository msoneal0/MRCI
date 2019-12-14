#ifndef CMDOBJECT_H
#define CMDOBJECT_H

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

#include "common.h"

class IPCWorker : public QObject
{
    Q_OBJECT

private slots:

    void rdFromIPC();

private:

    IdleTimer    *idleTimer;
    QLocalSocket *ipcSocket;
    quint32       flags;
    quint8        ipcTypeId;
    quint32       ipcDataSize;
    QString       pipeName;

public slots:

    void dataIn(const QByteArray &data, quint8 typeId);

public:

    explicit IPCWorker(const QString &pipe, QObject *parent = nullptr);

public slots:

    void connectIPC();

signals:

    void dataOut(const QByteArray &data, quint8 typeId);
    void ipcClosed();
    void ipcOpened();
    void termProc();
};

//----------------------

class CmdObject : public MemShare
{
    Q_OBJECT

protected:

    QTimer    *keepAliveTimer;
    IPCWorker *ipcWorker;
    quint32    flags;

    void    mainTxt(const QString &txt);
    void    errTxt(const QString &txt);
    void    privTxt(const QString &txt);
    void    bigTxt(const QString &txt);
    void    async(quint16 asyncId, quint8 asyncType, const QByteArray &data = QByteArray());
    void    postProc();
    QString libName();

    virtual void procIn(const QByteArray &, quint8) {}
    virtual void onTerminate() {}

protected slots:

    void preProc(const QByteArray &data, quint8 typeId);
    void keepAlive();
    void term();
    void kill();

    virtual void onIPCConnected() {}

public:

    explicit CmdObject(QObject *parent = nullptr);

signals:

    void procOut(const QByteArray &data, quint8 typeId);
};

#endif // CMDOBJECT_H
