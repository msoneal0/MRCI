#ifndef INT_LOADER_H
#define INT_LOADER_H

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

#include <QList>
#include <QPluginLoader>
#include <QSharedPointer>
#include <QHash>

#include "db.h"
#include "common.h"
#include "commands/command.h"
#include "commands/admin.h"
#include "commands/bans.h"
#include "commands/cast.h"
#include "commands/groups.h"
#include "commands/info.h"
#include "commands/mods.h"
#include "commands/users.h"
#include "commands/auth.h"
#include "commands/cmd_ranks.h"
#include "commands/cmd_state.h"
#include "commands/acct_recovery.h"
#include "commands/fs.h"
#include "commands/certs.h"
#include "commands/p2p.h"
#include "commands/channels.h"

class InternalCommandLoader : public CommandLoader
{
    Q_OBJECT

private:

    QStringList objNames;
    bool        pubReg;
    bool        emailConfirmation;
    bool        passwrdResets;

    void loadSettings();
    void makeDocHeader(const QString &path);
    void appendToDoc(const QString &path, const QString &cmdName, InternCommand *obj);

public:

    RWSharedObjs *rwShared;

    bool           exists(const QString &cmdName);
    QStringList    cmdList();
    QStringList    pubCmdList();
    QStringList    rankExemptList();
    InternCommand *cmdObj(const QString &name);

    explicit InternalCommandLoader(RWSharedObjs *sharedData, QObject *parent = nullptr);
};

#endif // INT_LOADER_H
