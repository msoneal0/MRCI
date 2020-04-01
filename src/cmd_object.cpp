#include "cmd_object.h"

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

IPCWorker::IPCWorker(const QString &pipe, QObject *parent) : QObject(parent)
{
    pipeName  = pipe;
    ipcSocket = new QLocalSocket(this);
    flags     = 0;

    connect(ipcSocket, &QLocalSocket::readyRead, this, &IPCWorker::rdFromIPC);
    connect(ipcSocket, &QLocalSocket::disconnected, this, &IPCWorker::ipcClosed);
    connect(ipcSocket, &QLocalSocket::connected, this, &IPCWorker::ipcOpened);
}

void IPCWorker::rdFromIPC()
{
    if (flags & FRAME_RDY)
    {
        if (ipcSocket->bytesAvailable() >= ipcDataSize)
        {
            emit dataOut(ipcSocket->read(ipcDataSize), ipcTypeId);

            flags ^= FRAME_RDY;

            rdFromIPC();
        }
    }
    else if (ipcSocket->bytesAvailable() >= (FRAME_HEADER_SIZE - 4))
    {
        auto header = ipcSocket->read(FRAME_HEADER_SIZE - 4);

        ipcTypeId   = static_cast<quint8>(header[0]);
        ipcDataSize = static_cast<quint32>(rdInt(header.mid(1, 3)));
        flags      |= FRAME_RDY;

        rdFromIPC();
    }
}

void IPCWorker::dataIn(const QByteArray &data, quint8 typeId)
{
    // format: [typeId][payload_len][payload]

    ipcSocket->write(wrInt(typeId, 8) + wrInt(data.size(), MAX_FRAME_BITS) + data);
}

void IPCWorker::connectIPC()
{
    ipcSocket->connectToServer(pipeName);
}

CmdObject::CmdObject(QObject *parent) : MemShare(parent)
{
    flags   = 0;
    retCode = NO_ERRORS;

    auto args    = QCoreApplication::instance()->arguments();
    auto pipe    = getParam("-pipe_name", args);
    auto sMemKey = getParam("-mem_ses", args);
    auto hMemKey = getParam("-mem_host", args);

    if (attachSharedMem(sMemKey, hMemKey))
    {
        ipcWorker      = new IPCWorker(pipe, nullptr);
        keepAliveTimer = new QTimer(this);
        progTimer      = new QTimer(this);
        progCurrent    = 0;
        progMax        = 0;

        auto *thr = new QThread(nullptr);

        serializeThread(thr);
        setupDataBlocks();

        connect(thr, &QThread::started, ipcWorker, &IPCWorker::connectIPC);

        connect(this, &CmdObject::destroyed, thr, &QThread::deleteLater);
        connect(this, &CmdObject::procOut, ipcWorker, &IPCWorker::dataIn);

        connect(keepAliveTimer, &QTimer::timeout, this, &CmdObject::keepAlive);
        connect(progTimer, &QTimer::timeout, this, &CmdObject::sendProg);

        connect(ipcWorker, &IPCWorker::dataOut, this, &CmdObject::preProc);
        connect(ipcWorker, &IPCWorker::termProc, this, &CmdObject::kill);
        connect(ipcWorker, &IPCWorker::ipcClosed, this, &CmdObject::term);
        connect(ipcWorker, &IPCWorker::ipcOpened, this, &CmdObject::onIPCConnected);

        keepAliveTimer->setSingleShot(false);
        keepAliveTimer->setInterval(30000); //30sec keep alive
        progTimer->setSingleShot(false);
        progTimer->setInterval(1000); // 1sec progress updater
        ipcWorker->moveToThread(thr);
        thr->start();
    }
    else
    {
        kill();
    }
}

void CmdObject::term()
{
    if (flags & (MORE_INPUT | YIELD_STATE | LOOPING))
    {
        flags   = 0;
        retCode = ABORTED;

        onTerminate();
        postProc();
    }
}

void CmdObject::kill()
{
    term();

    QCoreApplication::instance()->exit();
}

void CmdObject::preProc(const QByteArray &data, quint8 typeId)
{
    if (typeId == TERM_CMD)
    {
        term();
    }
    else if (typeId == KILL_CMD)
    {
        kill();
    }
    else if (typeId == YIELD_CMD)
    {
        if (flags & LOOPING)
        {
            flags |=  YIELD_STATE;
            flags &= ~LOOPING;
        }
    }
    else if (typeId == RESUME_CMD)
    {
        if (flags & YIELD_STATE)
        {
            flags |=  LOOPING;
            flags &= ~YIELD_STATE;
        }
    }
    else
    {
        sharedMem->lock();

        procIn(data, typeId);

        sharedMem->unlock();

        postProc();
    }
}

void CmdObject::postProc()
{
    if (flags & LOOPING)
    {
        preProc(QByteArray(), TEXT);
    }
    else if (flags & (MORE_INPUT | YIELD_STATE))
    {
        keepAliveTimer->start();
    }
    else
    {
        keepAliveTimer->stop();

        if (progTimer->isActive())
        {
            stopProgPulse();
        }

        emit procOut(wrInt(retCode, 16), IDLE);

        retCode = NO_ERRORS;
    }
}

void CmdObject::mainTxt(const QString &txt)
{
    emit procOut(toTEXT(txt), TEXT);
}

void CmdObject::errTxt(const QString &txt)
{
    emit procOut(toTEXT(txt), ERR);
}

void CmdObject::privTxt(const QString &txt)
{
    emit procOut(toTEXT(txt), PRIV_TEXT);
}

void CmdObject::bigTxt(const QString &txt)
{
    emit procOut(toTEXT(txt), BIG_TEXT);
}

void CmdObject::promptTxt(const QString &txt)
{
    emit procOut(toTEXT(txt), PROMPT_TEXT);
}

void CmdObject::async(quint16 asyncId, const QByteArray &data)
{
    emit procOut(wrInt(asyncId, 16) + data, ASYNC_PAYLOAD);
}

void CmdObject::keepAlive()
{
    async(ASYNC_KEEP_ALIVE);
}

void CmdObject::startProgPulse()
{
    progCurrent = 0;

    progTimer->start();
}

void CmdObject::stopProgPulse()
{
    progTimer->stop();

    emit procOut(wrInt(100, 8), PROG_LAST);

    progCurrent = 0;
    progMax     = 0;
}

void CmdObject::sendProg()
{
    quint8 percent = progCurrent / progMax * 100;

    if ((progMax == 0) || (percent >= 100))
    {
        stopProgPulse();
    }
    else
    {
        emit procOut(wrInt(percent, 8), PROG);
    }
}

QString CmdObject::libName()
{
    return QCoreApplication::applicationName() + " v" + QCoreApplication::applicationVersion() + " " + QString::number(QSysInfo::WordSize) + "Bit";
}
