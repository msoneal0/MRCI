#include "shell.h"

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

QStringList parseEnvVariables(const QString &txtIn)
{
    QStringList ret;
    QString     var;
    bool        varChr = false;

    for (auto&& chr : txtIn)
    {

#ifdef Q_OS_WIN

        if (chr == '%')
        {
            if (varChr && !var.isEmpty())
            {
                ret.append(var);
                var.clear();
            }

            varChr = !varChr;
        }

        if (varChr && (chr != '%'))
        {
            var.append(chr);
        }

#else
        if (varChr && !chr.isLetterOrNumber())
        {
            varChr = false;

            if (!var.isEmpty())
            {
                ret.append(var);
                var.clear();
            }
        }

        if ((chr == '$') && !varChr)
        {
            varChr = true;
        }
        else if (varChr)
        {
            var.append(chr);
        }

#endif

    }

    return ret;
}

QString expandEnvVariables(const QString &txtIn)
{
    QStringList vars = parseEnvVariables(txtIn);
    QString     ret  = txtIn;

    for (auto&& var : vars)
    {

#ifdef Q_OS_WIN

        ret.replace("%" + var + "%", qEnvironmentVariable(var.toUtf8(), var));

#else

        ret.replace("$" + var, qEnvironmentVariable(var.toUtf8(), var));

#endif

    }

    return ret;
}
