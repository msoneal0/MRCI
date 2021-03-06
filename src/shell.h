#ifndef SHELL_H
#define SHELL_H

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

#include <QProcess>
#include <QFile>
#include <QDir>
#include <QFileInfoList>
#include <QDebug>

QStringList parseEnvVariables(const QString &txtIn);
QString     expandEnvVariables(const QString &txtIn);
QString     parseMd(const QString &cmdName, int offset);
void        updateInternCmdList(const QString &dst);

#endif // SHELL_H
