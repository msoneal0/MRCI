#include "unix_signal.h"

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

#ifdef Q_OS_LINUX

void setupUnixSignalHandlers()
{
    struct sigaction hup;
    struct sigaction term;

    hup.sa_handler  = UnixSignalHandler::hupSignalHandler;
    term.sa_handler = UnixSignalHandler::termSignalHandler;

    sigemptyset(&hup.sa_mask);
    sigemptyset(&term.sa_mask);

    hup.sa_flags   = 0;
    hup.sa_flags  |= SA_RESTART;
    term.sa_flags |= SA_RESTART;

    sigaction(SIGHUP, &hup, 0);
    sigaction(SIGTERM, &term, 0);
}

int *UnixSignalHandler::sighupFd  = new int[2];
int *UnixSignalHandler::sigtermFd = new int[2];

UnixSignalHandler::UnixSignalHandler(QObject *parent) : QObject(parent)
{
    socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd);

    snHup  = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);

    connect(snHup,  SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
}

void UnixSignalHandler::hupSignalHandler(int)
{
    char chr = 1;

    write(sighupFd[0], &chr, sizeof(chr));
}

void UnixSignalHandler::termSignalHandler(int)
{
    char chr = 1;

    write(sigtermFd[0], &chr, sizeof(chr));
}

void UnixSignalHandler::handleSigTerm()
{
    snTerm->setEnabled(false);

    char chr;

    read(sigtermFd[1], &chr, sizeof(chr));

    emit closeServer();

    snTerm->setEnabled(true);
}

void UnixSignalHandler::handleSigHup()
{
    snHup->setEnabled(false);

    char chr;

    read(sighupFd[1], &chr, sizeof(chr));

    emit closeServer();

    snHup->setEnabled(true);
}

#endif // Q_OS_LINUX
