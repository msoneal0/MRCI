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

QString parseMd(const QString &cmdName, int offset)
{
    QFile      file(":/docs/intern_commands/" + cmdName + ".md");
    QByteArray data;

    if (!file.open(QFile::ReadOnly))
    {
        qDebug() << "err: internal command: " << cmdName << " does not have a document file.";
    }
    else
    {
        data = file.readAll();

        int targetTags = offset * 6;
        int pos        = -1;
        int len        = 0;

        for (int i = 0, tags = 0; i < data.size(); ++i)
        {
            if (data[i] == '#')
            {
                ++tags;

                if (pos != -1)
                {
                    break;
                }
            }
            else if (tags == targetTags)
            {
                len++;

                if (pos == -1)
                {
                    pos = i;
                }
            }
        }

        data = data.mid(pos, len).trimmed();

        if (offset == 2)
        {
            data.chop(3);
            data.remove(0, 3);
        }
    }

    file.close();

    return data;
}

void updateInternCmdList(const QString &dst)
{
    QString baseName = QFileInfo(dst).baseName();
    QString path     = QFileInfo(dst).path();

    QFile file(dst);
    QDir  lister(path + "/" + baseName);

    lister.mkpath(path + "/" + baseName);
    lister.setSorting(QDir::Name);

    if (file.open(QFile::WriteOnly | QFile::Truncate))
    {
        file.write("### 7.1 Internal Commands ###\n\n");
        file.write("The host is extendable via 3rd party modules but the host itself is an internal module that load commands with direct access to the host database.\n\n");

        QFileInfoList fileList = lister.entryInfoList();

        for (auto&& info : fileList)
        {
            QString line = "* [" + info.baseName() + "](" + baseName + "/" + info.fileName() + ") - " + parseMd(info.baseName(), 1) + "\n\n";

            file.write(line.toUtf8());
        }
    }

    file.close();
}
