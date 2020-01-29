#ifndef MODULE_H
#define MODULE_H

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

#include "common.h"
#include "cmd_object.h"
#include "commands/admin.h"
#include "commands/cast.h"
#include "commands/info.h"
#include "commands/mods.h"
#include "commands/users.h"
#include "commands/auth.h"
#include "commands/cmd_ranks.h"
#include "commands/acct_recovery.h"
#include "commands/fs.h"
#include "commands/certs.h"
#include "commands/p2p.h"
#include "commands/channels.h"

class Module : public QObject
{
    Q_OBJECT

private:

    bool pubReg;
    bool emailConfirmation;
    bool passwrdResets;

    bool        runCmd(const QString &name);
    void        listCmds(const QStringList &list);
    void        loadSettings();
    QStringList userCmdList();
    QStringList pubCmdList();
    QStringList rankExemptList();

public:

    explicit Module(QObject *parent = nullptr);

    bool start(const QStringList &args);
};

#endif // MODULE_H
