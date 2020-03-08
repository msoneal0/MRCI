#include <QCoreApplication>
#include <QAbstractSocket>
#include <QSharedPointer>
#include <QByteArray>
#include <QProcess>

#include "db.h"
#include "common.h"
#include "tcp_server.h"
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

void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(type)
    Q_UNUSED(context)

    if (!msg.contains("QSslSocket: cannot resolve"))
    {
        Query db;

        db.setType(Query::PUSH, TABLE_DMESG);
        db.addColumn(COLUMN_LOGENTRY, msg);
        db.exec();
    }
}

void showHelp()
{
    QTextStream txtOut(stdout);

    txtOut << "" << endl << APP_NAME << " v" << QCoreApplication::applicationVersion() << endl << endl;
    txtOut << "Usage: " << APP_TARGET << " <argument>" << endl << endl;
    txtOut << "<Arguments>" << endl << endl;
    txtOut << " -help                   : display usage information about this application." << endl;
    txtOut << " -stop                   : stop the current host instance if one is currently running." << endl;
    txtOut << " -about                  : display versioning/warranty information about this application." << endl;
    txtOut << " -addr {ip_address:port} : set the listening address and port for TCP clients." << endl;
    txtOut << " -status                 : display status information about the host instance if it is currently running." << endl;
    txtOut << " -reset_root             : reset the root account password to the default password." << endl;
    txtOut << " -host                   : start a new host instance. (this blocks)" << endl;
    txtOut << " -default_pw             : show the default password." << endl;
    txtOut << " -public_cmds            : run the internal module to list it's public commands. for internal use only." << endl;
    txtOut << " -exempt_cmds            : run the internal module to list it's rank exempt commands. for internal use only." << endl;
    txtOut << " -user_cmds              : run the internal module to list it's user commands. for internal use only." << endl;
    txtOut << " -run_cmd {command_name} : run an internal module command. for internal use only." << endl << endl;
    txtOut << "Internal module | -public_cmds, -user_cmds, -exempt_cmds, -run_cmd |:" << endl << endl;
    txtOut << " -pipe {pipe_name/path} : the named pipe used to establish a data connection with the session." << endl;
    txtOut << " -mem_ses {key_name}    : the shared memory key for the session." << endl;
    txtOut << " -mem_host {key_name}   : the shared memory key for the host main process." << endl << endl;
}

void soeDueToDbErr(int *retCode, const QString *errMsg)
{
    *retCode = 1;

    QTextStream(stderr) << "" << endl << "err: Stop error." << endl;
    QTextStream(stderr) << "     what happened: " << endl << *errMsg << endl << endl;
}

int shellToHost(const QStringList &args, QCoreApplication &app)
{
    auto  ret = 0;
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

    auto workDir = expandEnvVariables(qEnvironmentVariable(ENV_WORK_DIR, DEFAULT_WORK_DIR));
    auto args    = QCoreApplication::arguments();
    auto ret     = 0;

    QDir::setCurrent(workDir);
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VER);

    QString err;

    qInstallMessageHandler(msgHandler);

    //args.append("-host"); // debug

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
    else if (args.contains("-default_pw", Qt::CaseInsensitive))
    {
        QTextStream(stdout) << "" << endl << " Root User       : " << getUserName(rootUserId()) << endl;
        QTextStream(stdout) << " Default Password: " << defaultPw() << endl << endl;
    }
    else if (args.contains("-addr", Qt::CaseInsensitive))
    {
        auto params = getParam("-addr", args);
        auto addr   = params.split(':');

        ret = 128;

        if (!setupDb(&err))
        {
            soeDueToDbErr(&ret, &err);
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
    else if (args.contains("-run_cmd", Qt::CaseInsensitive)     ||
             args.contains("-public_cmds", Qt::CaseInsensitive) ||
             args.contains("-exempt_cmds", Qt::CaseInsensitive) ||
             args.contains("-user_cmds", Qt::CaseInsensitive))
    {
        if (!setupDb(&err))
        {
            soeDueToDbErr(&ret, &err);
        }
        else
        {
            auto *mod = new Module(&app);

            if (mod->start(args))
            {
                ret = QCoreApplication::exec();
            }
        }
    }
    else if (args.contains("-host", Qt::CaseInsensitive))
    {
        if (!setupDb(&err))
        {
            soeDueToDbErr(&ret, &err);
        }
        else
        {
            auto *serv = new TCPServer(&app);

            if (serv->start())
            {
                ret = QCoreApplication::exec();
            }
        }
    }
    else if (args.contains("-stop", Qt::CaseInsensitive) || args.contains("-status", Qt::CaseInsensitive))
    {
        ret = shellToHost(args, app);
    }
    else if (args.contains("-reset_root", Qt::CaseInsensitive))
    {
        if (!setupDb(&err))
        {
            soeDueToDbErr(&ret, &err);
        }
        else
        {
            auto uId = rootUserId();

            Query db(&app);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_LOCKED, false);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            updatePassword(uId, defaultPw(), TABLE_USERS, true);
        }
    }
    else
    {
        showHelp();
    }

    cleanupDbConnection();

    return ret;
}
