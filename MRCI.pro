#-------------------------------------------------
#
# Project created by QtCreator 2017-03-19
#
#    This file is part of MRCI.
#
#    MRCI is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    MRCI is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with MRCI under the LICENSE.md file. If not, see
#    <http://www.gnu.org/licenses/>.
#
#-------------------------------------------------

QT -= gui
QT += network
QT += sql

CONFIG += console
CONFIG -= app_bundle

TARGET = mrci

win32 {

  LIBS += -llibeay32 -lssleay32

} else {

  LIBS += -lcrypto -lssl

}

SOURCES += src/main.cpp \
           src/commands/channels.cpp \
           src/commands/cmd_ranks.cpp \
           src/commands/p2p.cpp \
           src/session.cpp \
           src/db.cpp \
           src/make_cert.cpp \
           src/int_loader.cpp \
           src/tcp_server.cpp \
           src/unix_signal.cpp \
           src/cmd_executor.cpp \
           src/common.cpp \
           src/shell.cpp \
           src/db_setup.cpp \
           src/commands/users.cpp \
           src/commands/mods.cpp \
           src/commands/info.cpp \
           src/commands/groups.cpp \
           src/commands/cast.cpp \
           src/commands/bans.cpp \
           src/commands/admin.cpp \
           src/commands/auth.cpp \
           src/commands/command.cpp \
           src/commands/cmd_state.cpp \
           src/commands/acct_recovery.cpp \
           src/commands/table_viewer.cpp \
           src/commands/fs.cpp \
           src/commands/certs.cpp

HEADERS += \
           src/commands/channels.h \
           src/commands/cmd_ranks.h \
           src/commands/p2p.h \
           src/session.h \
           src/db.h \
           src/make_cert.h \
           src/int_loader.h \
           src/tcp_server.h \
           src/unix_signal.h \
           src/cmd_executor.h \
           src/common.h \
           src/shell.h \
           src/db_setup.h \
           src/commands/users.h \
           src/commands/mods.h \
           src/commands/info.h \
           src/commands/groups.h \
           src/commands/cast.h \
           src/commands/bans.h \
           src/commands/admin.h \
           src/commands/auth.h \
           src/commands/command.h \
           src/commands/cmd_state.h \
           src/commands/acct_recovery.h \
           src/commands/table_viewer.h \
           src/commands/fs.h \
           src/commands/certs.h

RESOURCES += \
             cmd_docs.qrc
