#include "module.h"

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

Module::Module(QObject *parent) : QObject(parent)
{
    pubReg            = false;
    emailConfirmation = false;
    passwrdResets     = false;

    loadSettings();
}

void Module::loadSettings()
{
    Query db(this);

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_PUB_USERS);
    db.addColumn(COLUMN_ENABLE_CONFIRM);
    db.addColumn(COLUMN_ENABLE_PW_RESET);
    db.exec();

    pubReg            = db.getData(COLUMN_PUB_USERS).toBool();
    emailConfirmation = db.getData(COLUMN_ENABLE_CONFIRM).toBool();
    passwrdResets     = db.getData(COLUMN_ENABLE_PW_RESET).toBool();
}

QStringList Module::userCmdList()
{
    QStringList ret;

    ret << CloseHost::cmdName();
    ret << RestartHost::cmdName();
    ret << Cast::cmdName();
    ret << OpenSubChannel::cmdName();
    ret << CloseSubChannel::cmdName();
    ret << LsOpenChannels::cmdName();
    ret << HostInfo::cmdName();
    ret << IPHist::cmdName();
    ret << ListMods::cmdName();
    ret << DelMod::cmdName();
    ret << AddMod::cmdName();
    ret << ListUsers::cmdName();
    ret << CreateUser::cmdName();
    ret << RecoverAcct::cmdName();
    ret << ResetPwRequest::cmdName();
    ret << AuthLog::cmdName();
    ret << LsCmdRanks::cmdName();
    ret << RemoveCmdRank::cmdName();
    ret << AssignCmdRank::cmdName();
    ret << ServSettings::cmdName();
    ret << LockUser::cmdName();
    ret << NameChangeRequest::cmdName();
    ret << PasswordChangeRequest::cmdName();
    ret << OverWriteEmail::cmdName();
    ret << RemoveUser::cmdName();
    ret << ChangeUserRank::cmdName();
    ret << SetEmailTemplate::cmdName();
    ret << PreviewEmail::cmdName();
    ret << DownloadFile::cmdName();
    ret << UploadFile::cmdName();
    ret << Delete::cmdName();
    ret << Copy::cmdName();
    ret << Move::cmdName();
    ret << ListFiles::cmdName();
    ret << FileInfo::cmdName();
    ret << MakePath::cmdName();
    ret << ChangeDir::cmdName();
    ret << ListDBG::cmdName();
    ret << ListCerts::cmdName();
    ret << CertInfo::cmdName();
    ret << AddCert::cmdName();
    ret << RemoveCert::cmdName();
    ret << ToPeer::cmdName();
    ret << LsP2P::cmdName();
    ret << P2POpen::cmdName();
    ret << P2PClose::cmdName();
    ret << P2PRequest::cmdName();
    ret << PingPeers::cmdName();
    ret << CreateChannel::cmdName();
    ret << RemoveChannel::cmdName();
    ret << RenameChannel::cmdName();
    ret << SetActiveState::cmdName();
    ret << CreateSubCh::cmdName();
    ret << RemoveSubCh::cmdName();
    ret << RenameSubCh::cmdName();
    ret << ListChannels::cmdName();
    ret << ListSubCh::cmdName();
    ret << SearchChannels::cmdName();
    ret << InviteToCh::cmdName();
    ret << DeclineChInvite::cmdName();
    ret << AcceptChInvite::cmdName();
    ret << RemoveChMember::cmdName();
    ret << SetMemberLevel::cmdName();
    ret << SetSubAcessLevel::cmdName();
    ret << ListMembers::cmdName();
    ret << AddRDOnlyFlag::cmdName();
    ret << RemoveRDOnlyFlag::cmdName();
    ret << ListRDonlyFlags::cmdName();
    ret << OwnerOverride::cmdName();
    ret << Tree::cmdName();

    return ret + rankExemptList();
}

QStringList Module::pubCmdList()
{
    QStringList ret;

    ret << Auth::cmdName();
    ret << MyInfo::cmdName();

    if (pubReg)
    {
        ret << CreateUser::cmdName();
    }

    if (passwrdResets)
    {
        ret << ResetPwRequest::cmdName();
        ret << RecoverAcct::cmdName();
    }

    return ret;
}

QStringList Module::rankExemptList()
{
    QStringList ret;

    ret << Auth::cmdName();
    ret << MyInfo::cmdName();
    ret << ChangeDispName::cmdName();
    ret << ChangeUsername::cmdName();
    ret << ChangePassword::cmdName();
    ret << ChangeEmail::cmdName();
    ret << IsEmailVerified::cmdName();

    if (emailConfirmation)
    {
        ret << VerifyEmail::cmdName();
    }

    return ret;
}

bool Module::runCmd(const QString &name)
{
    bool ret = true;

    if (userCmdList().contains(name, Qt::CaseInsensitive))
    {
        if      (noCaseMatch(name, CloseHost::cmdName()))             new CloseHost(this);
        else if (noCaseMatch(name, RestartHost::cmdName()))           new RestartHost(this);
        else if (noCaseMatch(name, Auth::cmdName()))                  new Auth(this);
        else if (noCaseMatch(name, Cast::cmdName()))                  new Cast(this);
        else if (noCaseMatch(name, OpenSubChannel::cmdName()))        new OpenSubChannel(this);
        else if (noCaseMatch(name, CloseSubChannel::cmdName()))       new CloseSubChannel(this);
        else if (noCaseMatch(name, LsOpenChannels::cmdName()))        new LsOpenChannels(this);
        else if (noCaseMatch(name, HostInfo::cmdName()))              new HostInfo(this);
        else if (noCaseMatch(name, IPHist::cmdName()))                new IPHist(this);
        else if (noCaseMatch(name, ListMods::cmdName()))              new ListMods(this);
        else if (noCaseMatch(name, DelMod::cmdName()))                new DelMod(this);
        else if (noCaseMatch(name, AddMod::cmdName()))                new AddMod(this);
        else if (noCaseMatch(name, ListUsers::cmdName()))             new ListUsers(this);
        else if (noCaseMatch(name, CreateUser::cmdName()))            new CreateUser(this);
        else if (noCaseMatch(name, RecoverAcct::cmdName()))           new RecoverAcct(this);
        else if (noCaseMatch(name, ResetPwRequest::cmdName()))        new ResetPwRequest(this);
        else if (noCaseMatch(name, VerifyEmail::cmdName()))           new VerifyEmail(this);
        else if (noCaseMatch(name, AuthLog::cmdName()))               new AuthLog(this);
        else if (noCaseMatch(name, LsCmdRanks::cmdName()))            new LsCmdRanks(this);
        else if (noCaseMatch(name, RemoveCmdRank::cmdName()))         new RemoveCmdRank(this);
        else if (noCaseMatch(name, AssignCmdRank::cmdName()))         new AssignCmdRank(this);
        else if (noCaseMatch(name, ServSettings::cmdName()))          new ServSettings(this);
        else if (noCaseMatch(name, LockUser::cmdName()))              new LockUser(this);
        else if (noCaseMatch(name, NameChangeRequest::cmdName()))     new NameChangeRequest(this);
        else if (noCaseMatch(name, PasswordChangeRequest::cmdName())) new PasswordChangeRequest(this);
        else if (noCaseMatch(name, ChangeEmail::cmdName()))           new ChangeEmail(this);
        else if (noCaseMatch(name, OverWriteEmail::cmdName()))        new OverWriteEmail(this);
        else if (noCaseMatch(name, ChangeDispName::cmdName()))        new ChangeDispName(this);
        else if (noCaseMatch(name, ChangeUsername::cmdName()))        new ChangeUsername(this);
        else if (noCaseMatch(name, ChangePassword::cmdName()))        new ChangePassword(this);
        else if (noCaseMatch(name, RemoveUser::cmdName()))            new RemoveUser(this);
        else if (noCaseMatch(name, ChangeUserRank::cmdName()))        new ChangeUserRank(this);
        else if (noCaseMatch(name, IsEmailVerified::cmdName()))       new IsEmailVerified(this);
        else if (noCaseMatch(name, SetEmailTemplate::cmdName()))      new SetEmailTemplate(this);
        else if (noCaseMatch(name, PreviewEmail::cmdName()))          new PreviewEmail(this);
        else if (noCaseMatch(name, MyInfo::cmdName()))                new MyInfo(this);
        else if (noCaseMatch(name, DownloadFile::cmdName()))          new DownloadFile(this);
        else if (noCaseMatch(name, UploadFile::cmdName()))            new UploadFile(this);
        else if (noCaseMatch(name, Delete::cmdName()))                new Delete(this);
        else if (noCaseMatch(name, Copy::cmdName()))                  new Copy(this);
        else if (noCaseMatch(name, Move::cmdName()))                  new Move(this);
        else if (noCaseMatch(name, ListFiles::cmdName()))             new ListFiles(this);
        else if (noCaseMatch(name, FileInfo::cmdName()))              new FileInfo(this);
        else if (noCaseMatch(name, MakePath::cmdName()))              new MakePath(this);
        else if (noCaseMatch(name, ChangeDir::cmdName()))             new ChangeDir(this);
        else if (noCaseMatch(name, ListDBG::cmdName()))               new ListDBG(this);
        else if (noCaseMatch(name, ListCerts::cmdName()))             new ListCerts(this);
        else if (noCaseMatch(name, CertInfo::cmdName()))              new CertInfo(this);
        else if (noCaseMatch(name, AddCert::cmdName()))               new AddCert(this);
        else if (noCaseMatch(name, RemoveCert::cmdName()))            new RemoveCert(this);
        else if (noCaseMatch(name, ToPeer::cmdName()))                new ToPeer(this);
        else if (noCaseMatch(name, LsP2P::cmdName()))                 new LsP2P(this);
        else if (noCaseMatch(name, P2POpen::cmdName()))               new P2POpen(this);
        else if (noCaseMatch(name, P2PClose::cmdName()))              new P2PClose(this);
        else if (noCaseMatch(name, P2PRequest::cmdName()))            new P2PRequest(this);
        else if (noCaseMatch(name, PingPeers::cmdName()))             new PingPeers(this);
        else if (noCaseMatch(name, CreateChannel::cmdName()))         new CreateChannel(this);
        else if (noCaseMatch(name, RemoveChannel::cmdName()))         new RemoveChannel(this);
        else if (noCaseMatch(name, RenameChannel::cmdName()))         new RenameChannel(this);
        else if (noCaseMatch(name, SetActiveState::cmdName()))        new SetActiveState(this);
        else if (noCaseMatch(name, CreateSubCh::cmdName()))           new CreateSubCh(this);
        else if (noCaseMatch(name, RemoveSubCh::cmdName()))           new RemoveSubCh(this);
        else if (noCaseMatch(name, RenameSubCh::cmdName()))           new RenameSubCh(this);
        else if (noCaseMatch(name, ListChannels::cmdName()))          new ListChannels(this);
        else if (noCaseMatch(name, ListSubCh::cmdName()))             new ListSubCh(this);
        else if (noCaseMatch(name, SearchChannels::cmdName()))        new SearchChannels(this);
        else if (noCaseMatch(name, InviteToCh::cmdName()))            new InviteToCh(this);
        else if (noCaseMatch(name, DeclineChInvite::cmdName()))       new DeclineChInvite(this);
        else if (noCaseMatch(name, AcceptChInvite::cmdName()))        new AcceptChInvite(this);
        else if (noCaseMatch(name, RemoveChMember::cmdName()))        new RemoveChMember(this);
        else if (noCaseMatch(name, SetMemberLevel::cmdName()))        new SetMemberLevel(this);
        else if (noCaseMatch(name, SetSubAcessLevel::cmdName()))      new SetSubAcessLevel(this);
        else if (noCaseMatch(name, ListMembers::cmdName()))           new ListMembers(this);
        else if (noCaseMatch(name, AddRDOnlyFlag::cmdName()))         new AddRDOnlyFlag(this);
        else if (noCaseMatch(name, RemoveRDOnlyFlag::cmdName()))      new RemoveRDOnlyFlag(this);
        else if (noCaseMatch(name, ListRDonlyFlags::cmdName()))       new ListRDonlyFlags(this);
        else if (noCaseMatch(name, OwnerOverride::cmdName()))         new OwnerOverride(this);
        else if (noCaseMatch(name, Tree::cmdName()))                  new Tree(this);
        else
        {
            qDebug() << "Module err: the loader claims command name '" << name << "' exists but no command object was actually matched/built.";

            ret = false;
        }
    }
    else
    {
        qDebug() << "Module err: command name '" << name << "' not found.";

        ret = false;
    }

    return ret;
}

void Module::listCmds(const QStringList &list)
{
    new ListCommands(list, this);
}

bool Module::start(const QStringList &args)
{
    bool ret = true;

    if (args.contains("-run_cmd"))
    {   
        ret = runCmd(getParam("-run_cmd", args));
    }
    else if (args.contains("-public_cmds"))
    {
        listCmds(pubCmdList());
    }
    else if (args.contains("-exempt_cmds"))
    {
        listCmds(rankExemptList());
    }
    else if (args.contains("-user_cmds"))
    {
        listCmds(userCmdList());
    }
    else
    {
        ret = false;
    }

    return ret;
}
