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

#ifdef Q_OS_WINDOWS
extern "C"
{
// applink.c was copied from the openssl lib and slightly modified so
// it can be compiled in mingw64.
// per https://www.openssl.org/docs/man1.1.0/man3/OPENSSL_Applink.html
// this file provides a glue between OpenSSL BIO layer and Win32
// compiler run-time environment. without this the app will crash with
// a "no OPENSSL_Applink" error.
#include <src/applink.c>
}
#endif

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

    txtOut << "" << Qt::endl << APP_NAME << " v" << QCoreApplication::applicationVersion() << Qt::endl << Qt::endl;
    txtOut << "Usage: " << APP_TARGET << " <argument>" << Qt::endl << Qt::endl;
    txtOut << "<Arguments>" << Qt::endl << Qt::endl;
    txtOut << " -help        : display usage information about this application." << Qt::endl;
    txtOut << " -stop        : stop the current host instance if one is currently running." << Qt::endl;
    txtOut << " -about       : display versioning/warranty information about this application." << Qt::endl;
    txtOut << " -addr        : set the listening address and port for TCP clients." << Qt::endl;
    txtOut << " -status      : display status information about the host instance if it is currently running." << Qt::endl;
    txtOut << " -reset_root  : reset the root account password to the default password." << Qt::endl;
    txtOut << " -host        : start a new host instance. (this blocks)" << Qt::endl;
    txtOut << " -host_trig   : start a new host instance. (this does not block)" << Qt::endl;
    txtOut << " -default_pw  : show the default password." << Qt::endl;
    txtOut << " -public_cmds : run the internal module to list it's public commands. for internal use only." << Qt::endl;
    txtOut << " -exempt_cmds : run the internal module to list it's rank exempt commands. for internal use only." << Qt::endl;
    txtOut << " -user_cmds   : run the internal module to list it's user commands. for internal use only." << Qt::endl;
    txtOut << " -run_cmd     : run an internal module command. for internal use only." << Qt::endl;
    txtOut << " -ls_sql_drvs : list all available SQL drivers that the host currently supports." << Qt::endl;
    txtOut << " -load_ssl    : re-load the host SSL certificate without stopping the host instance." << Qt::endl << Qt::endl;
    txtOut << "Internal module | -public_cmds, -user_cmds, -exempt_cmds, -run_cmd |:" << Qt::endl << Qt::endl;
    txtOut << " -pipe     : the named pipe used to establish a data connection with the session." << Qt::endl;
    txtOut << " -mem_ses  : the shared memory key for the session." << Qt::endl;
    txtOut << " -mem_host : the shared memory key for the host main process." << Qt::endl << Qt::endl;
    txtOut << "Details:" << Qt::endl << Qt::endl;
    txtOut << "addr     - this argument takes a {ip_address:port} string. it will return an error if not formatted correctly" << Qt::endl;
    txtOut << "           examples: 10.102.9.2:35516 or 0.0.0.0:35516." << Qt::endl << Qt::endl;
    txtOut << "run_cmd  - this argument is used by the host itself along with the internal module arguments to run the" << Qt::endl;
    txtOut << "           internal command names passed by it. this is not ment to be run directly by human input. the" << Qt::endl;
    txtOut << "           executable will auto close if it fails to connect to the pipe and/or shared memory segments" << Qt::endl << Qt::endl;
}

void soeDueToDbErr(int *retCode, const QString *errMsg)
{
    *retCode = 1;

    QTextStream(stderr) << "" << Qt::endl << "err: Stop error." << Qt::endl;
    QTextStream(stderr) << "     what happened: " << Qt::endl << *errMsg << Qt::endl << Qt::endl;
}

int shellToHost(const QStringList &args, bool holdErrs, QCoreApplication &app)
{
    auto  ret = 0;
    auto *ipc = new ShellIPC(args, holdErrs, &app);

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

    QDir dir(workDir);

    if (!dir.exists()) dir.mkpath(workDir);

    QDir::setCurrent(workDir);
    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VER);

    QString err;

    qInstallMessageHandler(msgHandler);

    // args.append("-ls_sql_drvs"); // debug

    if (args.contains("-run_cmd", Qt::CaseInsensitive)     ||
        args.contains("-public_cmds", Qt::CaseInsensitive) ||
        args.contains("-exempt_cmds", Qt::CaseInsensitive) ||
        args.contains("-user_cmds", Qt::CaseInsensitive))
    {
        // security note: it is critical that the above internal arguments are checked
        //                first. external clients have the ability to pass additional
        //                args and those args come through here. it can be a security
        //                threat if an external arg is a powerful arg like "reset_root"
        //                and it ends up getting processed unintentionally by this
        //                function.

        if (setupDb(&err))
        {
            auto *mod = new Module(&app);

            if (mod->start(args))
            {
                ret = QCoreApplication::exec();
            }
        }
        else
        {
            soeDueToDbErr(&ret, &err);
        }
    }
    else if (args.contains("-help", Qt::CaseInsensitive) || args.size() == 1)
    {
        showHelp();
    }
    else if (args.contains("-ls_sql_drvs", Qt::CaseInsensitive))
    {
        QTextStream(stdout) << "" << Qt::endl;

        for (auto driver : QSqlDatabase::drivers())
        {
            QTextStream(stdout) << driver << Qt::endl;
        }

        QTextStream(stdout) << "" << Qt::endl;
    }
    else if (args.contains("-about", Qt::CaseInsensitive))
    {
        QTextStream(stdout) << "" << Qt::endl << APP_NAME << " v" << QCoreApplication::applicationVersion() << Qt::endl << Qt::endl;
        QTextStream(stdout) << "Based on QT " << QT_VERSION_STR << " " << 8 * QT_POINTER_SIZE << "bit" << Qt::endl << Qt::endl;
        QTextStream(stdout) << "The program is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE" << Qt::endl;
        QTextStream(stdout) << "WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE." << Qt::endl << Qt::endl;
    }
    else if (args.contains("-stop", Qt::CaseInsensitive)   ||
             args.contains("-status", Qt::CaseInsensitive) ||
             args.contains("-load_ssl", Qt::CaseInsensitive))
    {
        ret = shellToHost(args, false, app);
    }
    else if (setupDb(&err))
    {
        if (args.contains("-host", Qt::CaseInsensitive))
        {
            auto *serv = new TCPServer(&app);

            if (serv->start())
            {
                ret = QCoreApplication::exec();
            }
        }
        else if (args.contains("-host_trig", Qt::CaseInsensitive))
        {
            QProcess::startDetached(QCoreApplication::applicationFilePath(), QStringList() << "-host");
        }
        else if (args.contains("-addr", Qt::CaseInsensitive))
        {
            auto params = getParam("-addr", args);
            auto addr   = params.split(':');

            ret = 128;

            if (addr.size() != 2)
            {
                QTextStream(stderr) << "" << Qt::endl << "err: Address string parsing error, number of params found: " << addr.size() << Qt::endl;
            }
            else
            {
                bool pOk;
                auto port = addr[1].toUShort(&pOk);

                if (!pOk)
                {
                    QTextStream(stderr) << "" << Qt::endl << "err: Invalid port." << Qt::endl;
                }
                else if (port == 0)
                {
                    QTextStream(stderr) << "" << Qt::endl << "err: The port cannot be 0." << Qt::endl;
                }
                else if (QHostAddress(addr[0]).isNull())
                {
                    QTextStream(stderr) << "" << Qt::endl << "err: Invalid ip address." << Qt::endl;
                }
                else
                {
                    Query db(&app);

                    db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);
                    db.addColumn(COLUMN_IPADDR, addr[0]);
                    db.addColumn(COLUMN_PORT, port);
                    db.exec();

                    ret = shellToHost(args, true, app);
                }
            }
        }
        else if (args.contains("-reset_root", Qt::CaseInsensitive))
        {
            auto uId = rootUserId();

            Query db(&app);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_LOCKED, false);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            updatePassword(uId, defaultPw(), TABLE_USERS, true);
        }
        else if (args.contains("-default_pw", Qt::CaseInsensitive))
        {
            QTextStream(stdout) << "" << Qt::endl << " Root User       : " << getUserName(rootUserId()) << Qt::endl;
            QTextStream(stdout) << " Default Password: " << defaultPw() << Qt::endl << Qt::endl;
        }
    }
    else
    {
        soeDueToDbErr(&ret, &err);
    }

    cleanupDbConnection();

    return ret;
}
