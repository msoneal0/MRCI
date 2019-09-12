#include "tcp_server.h"

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

ModDeleteTimer::ModDeleteTimer(const QString &mod, QStringList *queue, QObject *parent) : QTimer (parent)
{
    delQueue = queue;
    modName  = mod;

    setInterval(5000); // 5 seconds

    queue->append(mod);

    connect(this, &ModDeleteTimer::timeout, this, &ModDeleteTimer::delMod);
}

void ModDeleteTimer::resetTimer(const QString &mod)
{
    if (modName == mod)
    {
        start();
    }
}

void ModDeleteTimer::delMod()
{
    Query db(this);

    db.setType(Query::PULL, TABLE_MODULES);
    db.addColumn(COLUMN_MOD_MAIN);
    db.addCondition(COLUMN_MOD_NAME, modName);
    db.exec();

    if (db.rows())
    {
        QString file = db.getData(COLUMN_MOD_MAIN).toString();

        QDir(QFileInfo(file).path()).removeRecursively();

        db.setType(Query::DEL, TABLE_MODULES);
        db.addCondition(COLUMN_MOD_NAME, modName);
        db.exec();
    }

    delQueue->removeAll(modName);

    deleteLater();
}

TCPServer::TCPServer(QObject *parent) : QTcpServer(parent)
{
    sessionCounter = new QSharedMemory(sessionCountShareKey(), this);
    controlPipe    = new QLocalServer(this);
    controlSocket  = nullptr;
    flags          = 0;

    sessionCounter->create(4);
    sessionCounter->attach();

    connect(controlPipe, &QLocalServer::newConnection, this, &TCPServer::newPipeConnection);
}

void TCPServer::newPipeConnection()
{
    if (controlSocket == nullptr)
    {
        controlSocket = controlPipe->nextPendingConnection();

        connect(controlSocket, &QLocalSocket::readyRead, this, &TCPServer::procPipeIn);
        connect(controlSocket, &QLocalSocket::disconnected, this, &TCPServer::closedPipeConnection);
    }
    else
    {
        controlPipe->nextPendingConnection()->deleteLater();
    }
}

void TCPServer::closedPipeConnection()
{
    controlSocket->deleteLater();

    controlSocket = nullptr;
}


bool TCPServer::start()
{
    bool ret = false;

    QString contrPath = pipesPath() + "/" + QString(APP_NAME) + ".TCPServer.Control";

    if (QFile::exists(contrPath))
    {
        QFile::remove(contrPath);
    }

    if (!controlPipe->listen(contrPath))
    {
        QTextStream(stderr) << "" << endl << "err: Unable to open a control pipe." << endl;
        QTextStream(stderr) << "err: Reason - " << controlPipe->errorString() << endl;
    }
    else
    {
        close();
        cleanupDbConnection();

        Query db(this);

        db.setType(Query::PULL, TABLE_SERV_SETTINGS);
        db.addColumn(COLUMN_PORT);
        db.addColumn(COLUMN_IPADDR);
        db.addColumn(COLUMN_MAXSESSIONS);
        db.exec();

        QString addr = db.getData(COLUMN_IPADDR).toString();
        quint16 port = static_cast<quint16>(db.getData(COLUMN_PORT).toUInt());

        if (listen(QHostAddress(addr), port))
        {
            ret = true;

            flags |= ACCEPTING;
        }
        else
        {
            QTextStream(stderr) << "" << endl << "err: TCP listen failure on address: " << addr << " port: " << port << endl;
            QTextStream(stderr) << "err: Reason - " << errorString() << endl;
        }
    }

    return ret;
}

void TCPServer::sessionEnded()
{
    uint count = rdSessionLoad() - 1;

    wrSessionLoad(count);

    if ((count == 0) && (flags & CLOSE_ON_EMPTY))
    {
        closeServer();
    }
    else if ((count == 0) && (flags & RES_ON_EMPTY))
    {
        resServer();
    }
    else if (!(flags & (CLOSE_ON_EMPTY | RES_ON_EMPTY)) && !servOverloaded())
    {
        resumeAccepting();
    }
}

void TCPServer::closeServer()
{
    close();

    if (rdSessionLoad() == 0)
    {
        cleanupDbConnection();

        controlPipe->close();
        sessionCounter->detach();

        QCoreApplication::instance()->quit();
    }
    else
    {
        flags |=  CLOSE_ON_EMPTY;
        flags &= ~RES_ON_EMPTY;

        emit endAllSessions();
    }
}

void TCPServer::resServer()
{
    if (rdSessionLoad() == 0)
    {
        controlPipe->close();

        start();
    }
    else
    {
        flags |=  RES_ON_EMPTY;
        flags &= ~CLOSE_ON_EMPTY;

        emit endAllSessions();
    }
}

bool TCPServer::servOverloaded()
{
    Query db(this);

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_MAXSESSIONS);
    db.exec();

    return rdSessionLoad() >= db.getData(COLUMN_MAXSESSIONS).toUInt();
}

void TCPServer::procPipeIn()
{
    QStringList args = parseArgs(controlSocket->readAll(), -1);

    if (args.contains("-stop", Qt::CaseInsensitive))
    {
        closeServer();

        controlSocket->write(toTEXT("\n"));
    }
    else if (args.contains("-status", Qt::CaseInsensitive))
    {
        QString     text;
        QTextStream txtOut(&text);

        Query db(this);

        db.setType(Query::PULL, TABLE_SERV_SETTINGS);
        db.addColumn(COLUMN_IPADDR);
        db.addColumn(COLUMN_PORT);
        db.addColumn(COLUMN_MAXSESSIONS);
        db.exec();

        txtOut << "" << endl;
        txtOut << "Host Load:            " << rdSessionLoad() << "/" << db.getData(COLUMN_MAXSESSIONS).toUInt() << endl;
        txtOut << "Active Address:       " << serverAddress().toString() << endl;
        txtOut << "Active Port:          " << serverPort() << endl;
        txtOut << "Set Address:          " << db.getData(COLUMN_IPADDR).toString() << endl;
        txtOut << "Set Port:             " << db.getData(COLUMN_PORT).toUInt() << endl;
        txtOut << "Database Path:        " << sqlDataPath() << endl;
        txtOut << "Modules Install Path: " << modDataPath() << endl << endl;

        controlSocket->write(toTEXT(text));
    }
}

bool TCPServer::inBanList(const QString &ip)
{
    Query db(this);

    db.setType(Query::PULL, TABLE_IPBANS);
    db.addColumn(COLUMN_IPADDR);
    db.addCondition(COLUMN_IPADDR, ip);
    db.exec();

    return db.rows();
}

void TCPServer::delayedModDel(const QString &modName)
{
    if (!modDelQueue.contains(modName))
    {
        auto *timer = new ModDeleteTimer(modName, &modDelQueue, this);

        connect(this, &TCPServer::resetModDelTimer, timer, &ModDeleteTimer::resetTimer);
    }

    emit resetModDelTimer(modName);
}

void TCPServer::incomingConnection(qintptr socketDescriptor)
{
    auto *soc = new QSslSocket(nullptr);

    soc->setSocketDescriptor(socketDescriptor);

    if (servOverloaded())
    {
        soc->deleteLater();

        pauseAccepting();
    }
    else
    {
        resumeAccepting();

        if (inBanList(soc->peerAddress().toString()))
        {
            soc->deleteLater();
        }
        else
        {
            auto *ses = new Session(nullptr);
            auto *thr = new QThread(nullptr);

            connect(thr, &QThread::finished, soc, &QSslSocket::deleteLater);
            connect(thr, &QThread::finished, ses, &QSslSocket::deleteLater);
            connect(thr, &QThread::finished, thr, &QSslSocket::deleteLater);

            connect(ses, &Session::ended, this, &TCPServer::sessionEnded);
            connect(ses, &Session::ended, thr, &QThread::quit);
            connect(ses, &Session::connectPeers, this, &TCPServer::connectPeers);
            connect(ses, &Session::delayedModDel, this, &TCPServer::delayedModDel);
            connect(ses, &Session::closeServer, this, &TCPServer::closeServer);
            connect(ses, &Session::resServer, this, &TCPServer::resServer);

            connect(this, &TCPServer::connectPeers, ses, &Session::connectToPeer);
            connect(this, &TCPServer::endAllSessions, ses, &Session::endSession);

            serializeThread(thr);

            ses->initAsMain(soc);
            ses->moveToThread(thr);
            soc->moveToThread(thr);
            thr->start();

            wrSessionLoad(rdSessionLoad() + 1);
        }
    }
}
