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
#define NEED_APPLINK
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
#else
#include <syslog.h>
#endif

#ifdef Q_OS_WINDOWS

void windowsLog(const QByteArray &id, const QByteArray &msg)
{
    auto file = QFile(getLocalFilePath(DEFAULT_LOG_FILENAME));
    auto date = QDateTime::currentDateTime();

    if (file.exists())
    {
        if (file.size() >= MAX_LOG_SIZE)
        {
            file.remove();

            windowsLog(id, msg);
        }
        else if (file.open(QIODevice::Append))
        {
            file.write(date.toString(Qt::ISODateWithMs).toUtf8() + ": msg_id: " + id + " " + msg + "\n");
        }
    }
    else
    {
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(date.toString(Qt::ISODateWithMs).toUtf8() + ": msg_id: " + id + " " + msg + "\n");
        }
    }

    file.close();
}

#endif

void msgHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)

    if (!msg.contains("QSslSocket: cannot resolve"))
    {
        auto logMsg = msg.toUtf8();

        switch (type)
        {
        case QtDebugMsg: case QtInfoMsg: case QtWarningMsg:
        {
            fprintf(stdout, "inf: %s\n", logMsg.constData());

#ifdef Q_OS_WINDOWS
            format.chop(2);

            windowsLog(format, "inf: " + utf8);
#else
            syslog(LOG_INFO, "inf: %s", logMsg.constData());
#endif
            break;
        }
        case QtCriticalMsg: case QtFatalMsg:
        {
            fprintf(stderr, "err: %s\n", logMsg.constData());

#ifdef Q_OS_WINDOWS
            format.chop(2);

            windowsLog(format, "err: " + utf8);
#else
            syslog(LOG_ERR, "err: %s", logMsg.constData());
#endif
            break;
        }
        }
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
    txtOut << " -status      : display status information about the host instance if it is currently running." << Qt::endl;
    txtOut << " -host        : start a new host instance. (this blocks)" << Qt::endl;
    txtOut << " -host_trig   : start a new host instance. (this does not block)" << Qt::endl;
    txtOut << " -public_cmds : run the internal module to list it's public commands. for internal use only." << Qt::endl;
    txtOut << " -exempt_cmds : run the internal module to list it's rank exempt commands. for internal use only." << Qt::endl;
    txtOut << " -user_cmds   : run the internal module to list it's user commands. for internal use only." << Qt::endl;
    txtOut << " -run_cmd     : run an internal module command. for internal use only." << Qt::endl;
    txtOut << " -ls_sql_drvs : list all available SQL drivers that the host currently supports." << Qt::endl;
    txtOut << " -load_ssl    : re-load the host SSL certificate without stopping the host instance." << Qt::endl;
    txtOut << " -elevate     : elevate any user account to rank 1." << Qt::endl;
    txtOut << " -res_pw      : reset a user account password with a randomized one time password." << Qt::endl;
    txtOut << " -add_admin   : create a rank 1 account with a randomized one time password." << Qt::endl << Qt::endl;
    txtOut << "Internal module | -public_cmds, -user_cmds, -exempt_cmds, -run_cmd |:" << Qt::endl << Qt::endl;
    txtOut << " -pipe     : the named pipe used to establish a data connection with the session." << Qt::endl;
    txtOut << " -mem_ses  : the shared memory key for the session." << Qt::endl;
    txtOut << " -mem_host : the shared memory key for the host main process." << Qt::endl << Qt::endl;
    txtOut << "Details:" << Qt::endl << Qt::endl;
    txtOut << "res_pw    - this argument takes a single string representing a user name to reset the password. the host" << Qt::endl;
    txtOut << "            will set a randomized password and display it on the CLI." << Qt::endl << Qt::endl;
    txtOut << "            example: -res_pw somebody" << Qt::endl << Qt::endl;
    txtOut << "add_admin - this argument takes a single string representing a user name to create a rank 1 account with." << Qt::endl;
    txtOut << "            the host will set a randomized password for it and display it on the CLI. this user will be" << Qt::endl;
    txtOut << "            required to change the password upon logging in." << Qt::endl;
    txtOut << "            example: -add_admin somebody" << Qt::endl << Qt::endl;
    txtOut << "elevate   - this argument takes a single string representing a user name to an account to promote to rank 1." << Qt::endl;
    txtOut << "            example: -elevate somebody" << Qt::endl << Qt::endl;
    txtOut << "run_cmd   - this argument is used by the host itself along with the internal module arguments to run the" << Qt::endl;
    txtOut << "            internal command names passed by it. this is not ment to be run directly by human input. the" << Qt::endl;
    txtOut << "            executable will auto close if it fails to connect to the pipe and/or shared memory segments" << Qt::endl << Qt::endl;
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

    auto args = QCoreApplication::arguments();
    auto ret  = 0;

    QCoreApplication::setApplicationName(APP_NAME);
    QCoreApplication::setApplicationVersion(APP_VER);

    // args.append("-ls_sql_drvs"); // debug

    if (args.contains("-run_cmd", Qt::CaseInsensitive)     ||
        args.contains("-public_cmds", Qt::CaseInsensitive) ||
        args.contains("-exempt_cmds", Qt::CaseInsensitive) ||
        args.contains("-user_cmds", Qt::CaseInsensitive))
    {
        if (setupDb())
        {
            auto *mod = new Module(&app);

            if (mod->start(args))
            {
                ret = QCoreApplication::exec();
            }
        }
        else
        {
            ret = 1;
        }
    }
    else if (args.contains("-help", Qt::CaseInsensitive) || args.size() == 1)
    {
        showHelp();
    }
    else if (args.contains("-ls_sql_drvs", Qt::CaseInsensitive))
    {
        QTextStream(stdout) << "" << Qt::endl;

        for (const auto &driver : QSqlDatabase::drivers())
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
        qInstallMessageHandler(msgHandler);

        ret = shellToHost(args, false, app);
    }
    else
    {
        qInstallMessageHandler(msgHandler);

        if (setupDb())
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
            else if (args.contains("-elevate", Qt::CaseInsensitive))
            {
                ret = 1;

                QByteArray uId;

                if (args.size() <= 2)
                {
                    QTextStream(stderr) << "err: A user name was not given." << Qt::endl;
                }
                else if (!validUserName(args[2]))
                {
                    QTextStream(stderr) << "err: Invalid user name." << Qt::endl;
                }
                else if (!userExists(args[2], &uId))
                {
                    QTextStream(stderr) << "err: The user name does not exists." << Qt::endl;
                }
                else
                {
                    Query db;

                    db.setType(Query::UPDATE, TABLE_USERS);
                    db.addColumn(COLUMN_HOST_RANK, 1);
                    db.addCondition(COLUMN_USER_ID, uId);

                    if (db.exec())
                    {
                        ret = 0;
                    }
                }
            }
            else if (args.contains("-add_admin", Qt::CaseInsensitive))
            {
                ret = 1;

                if (args.size() <= 2)
                {
                    QTextStream(stderr) << "err: A user name was not given." << Qt::endl;
                }
                else if (!validUserName(args[2]))
                {
                    QTextStream(stderr) << "err: Invalid user name." << Qt::endl;
                }
                else if (userExists(args[2]))
                {
                    QTextStream(stderr) << "err: The user name already exists." << Qt::endl;
                }
                else
                {
                    auto randPw = genPw();

                    if (createUser(args[2], args[2] + "@change_me.null", "", randPw, 1, true))
                    {
                        QTextStream(stdout) << "password: " << randPw << Qt::endl;

                        ret = 0;
                    }
                }
            }
            else if (args.contains("-res_pw", Qt::CaseInsensitive))
            {
                ret = 1;

                QByteArray uId;

                if (args.size() <= 2)
                {
                    QTextStream(stderr) << "err: A user name was not given." << Qt::endl;
                }
                else if (!validUserName(args[2]))
                {
                    QTextStream(stderr) << "err: Invalid user name." << Qt::endl;
                }
                else if (!userExists(args[2], &uId))
                {
                    QTextStream(stderr) << "err: The user name does not exists." << Qt::endl;
                }
                else
                {
                    auto randPw = genPw();

                    if (updatePassword(uId, randPw, TABLE_USERS, true))
                    {
                        QTextStream(stdout) << "password: " << randPw << Qt::endl;

                        ret = 0;
                    }
                }
            }
            else
            {
                showHelp();
            }
        }
        else
        {
            ret = 1;
        }

        cleanupDbConnection();
    }

    return ret;
}
