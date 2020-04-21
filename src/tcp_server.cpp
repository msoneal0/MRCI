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
    controlPipe   = new QLocalServer(this);
    hostSharedMem = new QSharedMemory(this);
    qNam          = new QNetworkAccessManager(this);
    hostKey       = createHostSharedMem(hostSharedMem);
    hostLoad      = static_cast<char*>(hostSharedMem->data());
    controlSocket = nullptr;
    flags         = 0;

#ifdef Q_OS_LINUX

    setupUnixSignalHandlers();

    auto *signalHandler = new UnixSignalHandler(QCoreApplication::instance());

    connect(signalHandler, &UnixSignalHandler::closeServer, this, &TCPServer::closeServer);

#endif

    connect(controlPipe, &QLocalServer::newConnection, this, &TCPServer::newPipeConnection);
    connect(qNam, &QNetworkAccessManager::finished, this, &TCPServer::replyFromIpify);
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

bool TCPServer::createPipe()
{
    auto ret = controlPipe->listen(HOST_CONTROL_PIPE);

    controlPipePath = controlPipe->fullServerName();

    if (!ret)
    {
        if (QFile::exists(controlPipePath))
        {
            QFile::remove(controlPipePath);
        }

        ret = controlPipe->listen(HOST_CONTROL_PIPE);
    }

    return ret;
}

void TCPServer::replyFromIpify(QNetworkReply *reply)
{
    wanIP = reply->readAll();

    loadSSLData(false);

    reply->deleteLater();
}

bool TCPServer::start()
{
    close();
    cleanupDbConnection();

    Query db(this);

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_PORT);
    db.addColumn(COLUMN_IPADDR);
    db.addColumn(COLUMN_MAXSESSIONS);
    db.exec();

    maxSessions = db.getData(COLUMN_MAXSESSIONS).toUInt();

    auto ret  = false;
    auto addr = db.getData(COLUMN_IPADDR).toString();
    auto port = static_cast<quint16>(db.getData(COLUMN_PORT).toUInt());

    if (!createPipe())
    {
        QTextStream(stderr) << "" << endl << "err: Unable to open a control pipe." << endl;
        QTextStream(stderr) << "err: Reason - " << controlPipe->errorString() << endl;
    }
    else if (!listen(QHostAddress(addr), port))
    {
        QTextStream(stderr) << "" << endl << "err: TCP listen failure on address: " << addr << " port: " << port << endl;
        QTextStream(stderr) << "err: Reason - " << errorString() << endl;
    }
    else if (hostKey.isEmpty())
    {
        QTextStream(stderr) << "" << endl << "err: Failed to create the host shared memory block." << endl;
        QTextStream(stderr) << "err: Reason - " << hostSharedMem->errorString() << endl;
    }
    else
    {
        qNam->get(QNetworkRequest(QUrl("https://api.ipify.org")));

        ret    = true;
        flags |= ACCEPTING;
    }

    return ret;
}

void TCPServer::sessionEnded()
{
    hostSharedMem->lock();

    quint32 count = rd32BitFromBlock(hostLoad) - 1;

    wr32BitToBlock(count, hostLoad);

    hostSharedMem->unlock();

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

    if (rd32BitFromBlock(hostLoad) == 0)
    {
        cleanupDbConnection();

        controlPipe->close();
        hostSharedMem->detach();

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
    if (rd32BitFromBlock(hostLoad) == 0)
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
    hostSharedMem->lock();

    bool ret = rd32BitFromBlock(hostLoad) >= maxSessions;

    hostSharedMem->unlock();

    return ret;
}

void TCPServer::procPipeIn()
{
    auto args = parseArgs(controlSocket->readAll(), -1);

    if (args.contains("-stop", Qt::CaseInsensitive))
    {
        closeServer();

        controlSocket->write(toTEXT("\n"));
    }
    else if (args.contains("-load_ssl", Qt::CaseInsensitive))
    {
        controlSocket->write(toTEXT(loadSSLData(true)));
    }
    else if (args.contains("-status", Qt::CaseInsensitive))
    {
        QString     text;
        QTextStream txtOut(&text);

        Query db(this);

        db.setType(Query::PULL, TABLE_SERV_SETTINGS);
        db.addColumn(COLUMN_IPADDR);
        db.addColumn(COLUMN_PORT);
        db.exec();

        hostSharedMem->lock();

        txtOut << "" << endl;
        txtOut << "Host Load:      " << rd32BitFromBlock(hostLoad) << "/" << maxSessions << endl;
        txtOut << "Active Address: " << serverAddress().toString() << endl;
        txtOut << "Active Port:    " << serverPort() << endl;
        txtOut << "Set Address:    " << db.getData(COLUMN_IPADDR).toString() << endl;
        txtOut << "Set Port:       " << db.getData(COLUMN_PORT).toUInt() << endl;
        txtOut << "Working Path:   " << QDir::currentPath() << endl;
        txtOut << "Database:       " << sqlDataPath() << endl;
        txtOut << "SSL Chain:      " << sslCertChain() << endl;
        txtOut << "SSL Private:    " << sslPrivKey() << endl << endl;

        hostSharedMem->unlock();
        controlSocket->write(toTEXT(text));
    }
}

void TCPServer::setMaxSessions(quint32 value)
{
    Query db(this);

    db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_MAXSESSIONS, value);
    db.exec();

    maxSessions = value;
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

        auto buffSize = static_cast<uint>(qPow(2, MAX_FRAME_BITS) - 1) + (MAX_FRAME_BITS / 8) + 4;
        //                                max_data_size_per_frame + size_of_size_bytes + size_of_cmd_id

        soc->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, buffSize);
        soc->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, buffSize);

        auto *ses = new Session(hostKey, soc, &sslKey, &sslChain, nullptr);
        auto *thr = new QThread(nullptr);

        connect(thr, &QThread::finished, soc, &QSslSocket::deleteLater);
        connect(thr, &QThread::finished, ses, &QSslSocket::deleteLater);
        connect(thr, &QThread::finished, thr, &QSslSocket::deleteLater);
        connect(thr, &QThread::started, ses, &Session::init);

        connect(ses, &Session::ended, this, &TCPServer::sessionEnded);
        connect(ses, &Session::ended, thr, &QThread::quit);
        connect(ses, &Session::connectPeers, this, &TCPServer::connectPeers);
        connect(ses, &Session::closeServer, this, &TCPServer::closeServer);
        connect(ses, &Session::resServer, this, &TCPServer::resServer);
        connect(ses, &Session::setMaxSessions, this, &TCPServer::setMaxSessions);

        connect(this, &TCPServer::connectPeers, ses, &Session::connectToPeer);
        connect(this, &TCPServer::endAllSessions, ses, &Session::endSession);

        serializeThread(thr);

        ses->moveToThread(thr);
        soc->moveToThread(thr);
        thr->start();

        hostSharedMem->lock();

        wr32BitToBlock((rd32BitFromBlock(hostLoad) + 1), hostLoad);

        hostSharedMem->unlock();
    }
}

void TCPServer::applyPrivKey(const QString &path, QTextStream &msg)
{
    auto bytes = rdFileContents(path, msg);

    if (!bytes.isEmpty())
    {
        msg << "Attempting to load the private key with RSA. ";

        QSslKey key(bytes, QSsl::Rsa);

        if (key.isNull())
        {
            msg << "[fail]" << endl;
            msg << "Attempting to load the private key with DSA. ";

            key = QSslKey(bytes, QSsl::Dsa);
        }

        if (key.isNull())
        {
            msg << "[fail]" << endl;
            msg << "Attempting to load the private key with Elliptic Curve. ";

            key = QSslKey(bytes, QSsl::Ec);
        }

        if (key.isNull())
        {
            msg << "[fail]" << endl;
            msg << "Attempting to load the private key with Diffie-Hellman. ";

            key = QSslKey(bytes, QSsl::Dh);
        }

        if (key.isNull())
        {
            msg << "[fail]" << endl;
            msg << "Attempting to load the private key as a black box. ";

            key = QSslKey(bytes, QSsl::Opaque);
        }

        if (key.isNull())
        {
            msg << "[fail]" << endl << endl;
        }
        else
        {
            msg << "[pass]" << endl << endl;

            sslKey = key;
        }
    }
}

void TCPServer::applyCerts(const QStringList &list, QTextStream &msg)
{
    sslChain.clear();

    for (auto file : list)
    {
        sslChain.append(QSslCertificate(rdFileContents(file, msg)));
    }
}

QString TCPServer::loadSSLData(bool onReload)
{
    QString     txtMsg;
    QTextStream stream(&txtMsg);

    auto chain          = sslCertChain().split(":");
    auto priv           = sslPrivKey();
    auto allCertsExists = true;
    auto privKeyExists  = QFile::exists(priv);

    stream << "Private key: " << priv << endl;

    if (!privKeyExists)
    {
        stream << "    ^(the private key does not exists)" << endl;
    }

    for (auto cert : chain)
    {
        stream << "Cert:        " << cert << endl;

        if (!QFile::exists(cert))
        {
            stream << "    ^(this cert does not exists)" << endl;

            allCertsExists = false;
        }
    }

    if (chain.isEmpty())
    {
        stream << "No cert files are defined in the env." << endl;

        allCertsExists = false;
    }

    stream << endl;

    if (allCertsExists && privKeyExists)
    {
        if (onReload && (priv == DEFAULT_PRIV_KEY_NAME) && (sslCertChain() == DEFAULT_PUB_KEY_NAME))
        {
            stream << "Re-generating self-signed cert." << endl;

            if (genDefaultSSLFiles(wanIP, stream))
            {
                stream << endl << "complete." << endl << endl;
            }
        }

        applyPrivKey(priv, stream);
        applyCerts(chain, stream);
    }
    else if ((priv == DEFAULT_PRIV_KEY_NAME) && (sslCertChain() == DEFAULT_PUB_KEY_NAME))
    {
        stream << "Generating self-signed cert." << endl;

        if (genDefaultSSLFiles(wanIP, stream))
        {
            stream << endl << "The default self-signed cert files are generated successfully." << endl << endl;

            applyPrivKey(priv, stream);
            applyCerts(chain, stream);
        }
    }

    return txtMsg;
}
