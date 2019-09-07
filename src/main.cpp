#include <QCoreApplication>
#include <QAbstractSocket>
#include <QSharedPointer>
#include <QByteArray>
#include <QProcess>

#include "db.h"
#include "common.h"
#include "tcp_server.h"
#include "unix_signal.h"
#include "cmd_executor.h"
#include "db_setup.h"

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

void showHelp()
{
    QTextStream txtOut(stdout);

    txtOut << "" << endl << APP_NAME << " v" << QCoreApplication::applicationVersion() << endl << endl;
    txtOut << "Usage: " << APP_TARGET << " <argument>" << endl << endl;
    txtOut << "<Arguments>" << endl << endl;
    txtOut << " -help                   : display usage information about this application." << endl;
    txtOut << " -start                  : start a new host instance in the background." << endl;
    txtOut << " -stop                   : stop the current host instance if one is currently running." << endl;
    txtOut << " -about                  : display versioning/warranty information about this application." << endl;
    txtOut << " -addr {ip_address:port} : set the listening address and port for TCP clients." << endl;
    txtOut << " -status                 : display status information about the host instance if it is currently running." << endl;
    txtOut << " -reset_root             : reset the root account password to default." << endl;
    txtOut << " -executor               : this starts a command executor instance. this is normally for internal use only." << endl;
    txtOut << " -host                   : this starts a blocking host instance. this is normally for internal use only." << endl << endl;
}

void soeDueToDbErr(int *retCode)
{
    *retCode = 1;

    QTextStream(stderr) << "" << endl << "err: Unable to continue due to an unclean or non-existent database structure." << endl;
}

int shellToHost(const QStringList &args, QCoreApplication &app)
{
    int   ret = 0;
    auto *ipc = new ShellIPC(args, &app);

    QObject::connect(ipc, SIGNAL(closeInstance()), &app, SLOT(quit()));

    if (ipc->connectToHost())
    {
        ret = QCoreApplication::exec();
    }

    return ret;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    qRegisterMetaType<QSharedPointer<QByteArray> >("QSharedPointer<QByteArray>");
    qRegisterMetaType<QSharedPointer<SessionCarrier> >("QSharedPointer<SessionCarrier>");

    serializeThread(app.thread());

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VER);

    qputenv(ENV_EXENAME, APP_TARGET);

    QString     err;
    QStringList args   = QCoreApplication::arguments();
    bool        dbFail = false;
    int         ret    = 0;

    if (!setupDb(&err))
    {
        QTextStream(stderr) << "" << endl << "err: Database setup error, the host will not be able to start without a solid database structure." << endl;
        QTextStream(stderr) << "     what happened: " << endl << err << endl;

        dbFail = true;
    }

    qInstallMessageHandler(msgHandler);

    if (args.contains("-help", Qt::CaseInsensitive))
    {
        showHelp();
    }
    else if (args.contains("-about", Qt::CaseInsensitive))
    {
        QTextStream(stdout) << "" << endl << APP_NAME << " v" << QCoreApplication::applicationVersion() << endl << endl;
        QTextStream(stdout) << "Based on QT " << QT_VERSION_STR << " " << 8 * QT_POINTER_SIZE << "bit" << endl << endl;
        QTextStream(stdout) << "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE" << endl;
        QTextStream(stdout) << "WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE." << endl << endl;
    }
    else if (args.contains("-addr", Qt::CaseInsensitive))
    {
        QString     params = getParam("-addr", args);
        QStringList addr   = params.split(':');

        ret = 128;

        if (dbFail)
        {
            soeDueToDbErr(&ret);
        }
        else if (addr.size() != 2)
        {
            QTextStream(stderr) << "" << endl << "err: Address string parsing error, number of params found: " << addr.size() << endl;
        }
        else
        {
            bool   pOk;
            ushort port = addr[1].toUShort(&pOk);

            if (!pOk)
            {
                QTextStream(stderr) << "" << endl << "err: Invalid port." << endl;
            }
            else if (port == 0)
            {
                QTextStream(stderr) << "" << endl << "err: The port cannot be 0." << endl;
            }
            else if (QHostAddress(addr[0]).isNull())
            {
                QTextStream(stderr) << "" << endl << "err: Invalid ip address." << endl;
            }
            else
            {
                ret = 0;

                Query db(&app);

                db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                db.addColumn(COLUMN_IPADDR, addr[0]);
                db.addColumn(COLUMN_PORT, port);
                db.exec();
            }
        }
    }
    else if (args.contains("-executor", Qt::CaseInsensitive))
    {
        if (dbFail)
        {
            soeDueToDbErr(&ret);
        }
        else
        {
            auto *session = new Session(&app);

            session->startAsSlave(args);

            ret = QCoreApplication::exec();
        }
    }
    else if (args.contains("-host", Qt::CaseInsensitive))
    {
        if (dbFail)
        {
            soeDueToDbErr(&ret);
        }
        else
        {
            auto *serv = new TCPServer(&app);

            #ifdef Q_OS_LINUX

            setupUnixSignalHandlers();

            auto *signalHandler = new UnixSignalHandler(&app);

            QObject::connect(signalHandler, SIGNAL(closeServer()), serv, SLOT(closeServer()));

            #endif

            if (serv->start())
            {
                ret = QCoreApplication::exec();
            }
        }
    }
    else if (args.contains("-start", Qt::CaseInsensitive))
    {
        if (dbFail)
        {
            soeDueToDbErr(&ret);
        }
        else
        {
            QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList() << "-host");
        }
    }
    else if (args.contains("-stop", Qt::CaseInsensitive) || args.contains("-status", Qt::CaseInsensitive))
    {
        ret = shellToHost(args, app);
    }
    else if (args.contains("-reset_root", Qt::CaseInsensitive))
    {
        if (dbFail)
        {
            soeDueToDbErr(&ret);
        }
        else
        {
            updatePassword(ROOT_USER, DEFAULT_PASSWRD, TABLE_USERS, true);
        }
    }
    else
    {
        showHelp();
    }

    cleanupDbConnection();

    return ret;
}
