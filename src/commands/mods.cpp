#include "mods.h"

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

ListMods::ListMods(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_MODULES, false);
    addTableColumn(TABLE_MODULES, COLUMN_MOD_MAIN);
}

AddMod::AddMod(QObject *parent) : CmdObject(parent) {}
DelMod::DelMod(QObject *parent) : CmdObject(parent) {}

QString ListMods::cmdName() {return "ls_mods";}
QString DelMod::cmdName()   {return "rm_mod";}
QString AddMod::cmdName()   {return "add_mod";}

bool AddMod::isExecutable(const QString &path)
{
    QFileInfo info(expandEnvVariables(path));

    return info.exists() && info.isExecutable();
}

void AddMod::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 2);
        auto path = getParam("-mod_path", args);

        retCode = INVALID_PARAMS;

        if (path.isEmpty())
        {
            errTxt("err: The path to module executable (-mod_path) argument not found or is empty.\n");
        }
        else if (!validModPath(path))
        {
            errTxt("err: The module path must be less then 512 chars long and cannot contain the following chars: :*?\"<>|\n");
        }
        else if (modExists(path))
        {
            errTxt("err: The module already exists.\n");
        }
        else if (!isExecutable(path))
        {
            errTxt("err: Executable: " + path + " does not exists or does not have execution permissions.\n");
        }
        else
        {
            retCode = NO_ERRORS;

            Query db(this);

            db.setType(Query::PUSH, TABLE_MODULES);
            db.addColumn(COLUMN_MOD_MAIN, path);
            db.exec();

            async(ASYNC_ENABLE_MOD, PUB_IPC_WITH_FEEDBACK, toTEXT(path));
        }
    }
}

void DelMod::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == TEXT)
    {
        auto args = parseArgs(binIn, 2);
        auto path = getParam("-mod_path", args);

        retCode = INVALID_PARAMS;

        if (path.isEmpty())
        {
            errTxt("err: The path to module executable (-mod_path) argument not found or is empty.\n");
        }
        else if (!validModPath(path))
        {
            errTxt("err: Invalid module path.\n");
        }
        else if (!modExists(path))
        {
            errTxt("err: No such module found: '" + path + "'\n");
        }
        else
        {
            retCode = NO_ERRORS;
            
            Query db(this);

            db.setType(Query::DEL, TABLE_MODULES);
            db.addCondition(COLUMN_MOD_MAIN, path);
            db.exec();

            async(ASYNC_DISABLE_MOD, PUB_IPC_WITH_FEEDBACK, toTEXT(path));
        }
    }
}
