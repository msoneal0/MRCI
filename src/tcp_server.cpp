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

    if (!ret)
    {
        auto controlPipePath = controlPipe->fullServerName();

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

    auto ret  = false;
    auto conf = confObject();
    auto addr = conf[CONF_LISTEN_ADDR].toString();
    auto port = conf[CONF_LISTEN_PORT].toInt();

    if (!createPipe())
    {
        QTextStream(stderr) << "" << Qt::endl << "err: Unable to open a control pipe." << Qt::endl;
        QTextStream(stderr) << "err: Reason - " << controlPipe->errorString() << Qt::endl;
    }
    else if (!listen(QHostAddress(addr), port))
    {
        QTextStream(stderr) << "" << Qt::endl << "err: TCP listen failure on address: " << addr << " port: " << port << Qt::endl;
        QTextStream(stderr) << "err: Reason - " << errorString() << Qt::endl;
    }
    else if (hostKey.isEmpty())
    {
        QTextStream(stderr) << "" << Qt::endl << "err: Failed to create the host shared memory block." << Qt::endl;
        QTextStream(stderr) << "err: Reason - " << hostSharedMem->errorString() << Qt::endl;
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

bool TCPServer::servOverloaded()
{
    hostSharedMem->lock();

    auto confObj = confObject();
    auto ret     = rd32BitFromBlock(hostLoad) >= static_cast<quint32>(confObj[CONF_MAX_SESSIONS].toInt());

    hostSharedMem->unlock();

    return ret;
}

void TCPServer::procPipeIn()
{
    auto args = parseArgs(controlSocket->readAll(), -1);

    if (args.contains("-stop", Qt::CaseInsensitive))
    {
        closeServer();

        controlSocket->write(QString("\n").toUtf8());
    }
    else if (args.contains("-load_ssl", Qt::CaseInsensitive))
    {
        controlSocket->write(loadSSLData(true).toUtf8());
    }
    else if (args.contains("-addr", Qt::CaseInsensitive))
    {
        auto params = getParam("-addr", args);
        auto addr   = params.split(':');

        close();

        QString     text;
        QTextStream txtOut(&text);

        if (!listen(QHostAddress(addr[0]), addr[1].toUInt()))
        {
            txtOut << "" << Qt::endl << "err: TCP listen failure on address: " << addr[0] << " port: " << addr[1] << Qt::endl;
            txtOut << "err: Reason - " << errorString() << Qt::endl;
        }

        txtOut << "" << Qt::endl;

        controlSocket->write(text.toUtf8());
    }
    else if (args.contains("-status", Qt::CaseInsensitive))
    {
        QString     text;
        QTextStream txtOut(&text);

        hostSharedMem->lock();

        auto confObj = confObject();

        txtOut << "" << Qt::endl;
        txtOut << "Host Load:   " << rd32BitFromBlock(hostLoad) << "/" << confObj[CONF_MAX_SESSIONS].toInt() << Qt::endl;
        txtOut << "Address:     " << serverAddress().toString() << Qt::endl;
        txtOut << "Port:        " << serverPort() << Qt::endl;
        txtOut << "SSL Chain:   " << confObj[CONF_CERT_CHAIN].toString() << Qt::endl;
        txtOut << "SSL Private: " << confObj[CONF_PRIV_KEY].toString() << Qt::endl << Qt::endl;

        printDatabaseInfo(txtOut);

        hostSharedMem->unlock();
        controlSocket->write(text.toUtf8());
    }
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
            msg << "[fail]" << Qt::endl;
            msg << "Attempting to load the private key with DSA. ";

            key = QSslKey(bytes, QSsl::Dsa);
        }

        if (key.isNull())
        {
            msg << "[fail]" << Qt::endl;
            msg << "Attempting to load the private key with Elliptic Curve. ";

            key = QSslKey(bytes, QSsl::Ec);
        }

        if (key.isNull())
        {
            msg << "[fail]" << Qt::endl;
            msg << "Attempting to load the private key with Diffie-Hellman. ";

            key = QSslKey(bytes, QSsl::Dh);
        }

        if (key.isNull())
        {
            msg << "[fail]" << Qt::endl;
            msg << "Attempting to load the private key as a black box. ";

            key = QSslKey(bytes, QSsl::Opaque);
        }

        if (key.isNull())
        {
            msg << "[fail]" << Qt::endl << Qt::endl;
        }
        else
        {
            msg << "[pass]" << Qt::endl << Qt::endl;

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

    auto localObj       = confObject();
    auto privPath       = localObj[CONF_PRIV_KEY].toString();
    auto pubPath        = localObj[CONF_CERT_CHAIN].toString();
    auto chain          = pubPath.split(":");
    auto allCertsExists = true;
    auto privKeyExists  = QFile::exists(privPath);

    stream << "Private key: " << privPath << Qt::endl;

    if (!privKeyExists)
    {
        stream << "    ^(the private key does not exists)" << Qt::endl;
    }

    for (auto cert : chain)
    {
        stream << "Cert:        " << cert << Qt::endl;

        if (!QFile::exists(cert))
        {
            stream << "    ^(this cert does not exists)" << Qt::endl;

            allCertsExists = false;
        }
    }

    if (chain.isEmpty())
    {
        stream << "No cert files are defined in the conf file." << Qt::endl;

        allCertsExists = false;
    }

    stream << Qt::endl;

    auto defaultPriv = getLocalFilePath(DEFAULT_PRIV_FILENAME);
    auto defaultCert = getLocalFilePath(DEFAULT_CERT_FILENAME);

    if (allCertsExists && privKeyExists)
    {
        if (onReload && (privPath == defaultPriv) && (pubPath == defaultCert))
        {
            stream << "Re-generating self-signed cert." << Qt::endl;

            if (genDefaultSSLFiles(wanIP, stream))
            {
                stream << Qt::endl << "complete." << Qt::endl << Qt::endl;
            }
        }

        applyPrivKey(privPath, stream);
        applyCerts(chain, stream);
    }
    else if ((privPath == defaultPriv) && (pubPath == defaultCert))
    {
        stream << "Generating self-signed cert." << Qt::endl;

        if (genDefaultSSLFiles(wanIP, stream))
        {
            stream << Qt::endl << "The default self-signed cert files were generated successfully." << Qt::endl << Qt::endl;

            applyPrivKey(privPath, stream);
            applyCerts(chain, stream);
        }
    }

    return txtMsg;
}
