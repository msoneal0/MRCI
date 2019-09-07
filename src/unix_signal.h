#ifndef UNIX_SIGNAL_H
#define UNIX_SIGNAL_H

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

#include <QtGlobal>

#ifdef Q_OS_LINUX

#include <QObject>
#include <QSocketNotifier>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

void setupUnixSignalHandlers();

class UnixSignalHandler : public QObject
{
    Q_OBJECT

private:

    QSocketNotifier *snHup;
    QSocketNotifier *snTerm;

    static int *sighupFd;
    static int *sigtermFd;

public:

    explicit UnixSignalHandler(QObject *parent = 0);

    static void hupSignalHandler(int);
    static void termSignalHandler(int);

public slots:

    void handleSigHup();
    void handleSigTerm();

signals:

    void closeServer();
};

#endif // Q_OS_LINUX
#endif // UNIX_SIGNAL_H
