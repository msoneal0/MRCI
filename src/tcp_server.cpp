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

TCPServer::TCPServer(QObject *parent) : QTcpServer(parent)
{
    sessionCounter = new QSharedMemory(sessionCountShareKey(), this);
    controlPipe    = new QLocalServer(this);
    dirDelTimer    = new QTimer(this);
    controlSocket  = nullptr;
    flags          = 0;

    sessionCounter->create(4);
    sessionCounter->attach();

    connect(controlPipe, &QLocalServer::newConnection, this, &TCPServer::newPipeConnection);
    connect(dirDelTimer, &QTimer::timeout, this, &TCPServer::delDir);
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

            syncModPath();
        }
        else
        {
            QTextStream(stderr) << "" << endl << "err: TCP listen failure on address: " << addr << " port: " << port << endl;
            QTextStream(stderr) << "err: Reason - " << errorString() << endl;
        }
    }

    return ret;
}

void TCPServer::syncModPath()
{
    QDir dir(modDataPath());

    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    QStringList modNames = dir.entryList();

    for (auto&& modName : modNames)
    {
        QString modPath = dir.absolutePath() + "/" + modName;

        if (!QFile::exists(modPath + "/main"))
        {
            QDir(modPath).removeRecursively();
        }
        else if (!modExists(modName))
        {
            Query db(this);

            db.setType(Query::PULL, TABLE_MODULES);
            db.addColumn(COLUMN_MOD_NAME);
            db.exec();

            quint16 idOffs = static_cast<quint16>((db.rows() + 2) * MAX_CMDS_PER_MOD);

            db.setType(Query::PUSH, TABLE_MODULES);
            db.addColumn(COLUMN_MOD_NAME, modName);
            db.addColumn(COLUMN_MOD_MAIN, modPath + "/main");
            db.addColumn(COLUMN_LOCKED, false);
            db.addColumn(COLUMN_CMD_ID_OFFS, idOffs);
            db.exec();
        }
    }
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

void TCPServer::delDir()
{
    if (dirsToDel.isEmpty())
    {
        dirDelTimer->stop();
    }
    else
    {
        QString path = dirsToDel.takeFirst();

        Query db(this);

        db.setType(Query::DEL, TABLE_MODULES);
        db.addCondition(COLUMN_MOD_MAIN, path, Query::LIKE);
        db.exec();

        QDir(path).removeRecursively();
    }
}

void TCPServer::delayedDirDel(const QString &path)
{
    dirDelTimer->setSingleShot(false);
    dirDelTimer->start(5000);

    dirsToDel.append(path);
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
            connect(ses, &Session::delayedDirDel, this, &TCPServer::delayedDirDel);
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
