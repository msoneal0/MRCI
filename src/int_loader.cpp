#include "int_loader.h"

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

InternalCommandLoader::InternalCommandLoader(RWSharedObjs *sharedData, QObject *parent) : CommandLoader(parent)
{
    rwShared          = sharedData;
    pubReg            = false;
    emailConfirmation = false;
    passwrdResets     = false;

    // This is used as an automated way to update the Internal_Commands doc for the source code documentation
    // keep it commented for release code.

    // makeDocHeader("/path/to/source/docs/Internal_Commands.md");

    objNames << CloseHost::cmdName();
    objNames << RestartHost::cmdName();
    objNames << Auth::cmdName();
    objNames << ListBans::cmdName();
    objNames << BanIP::cmdName();
    objNames << UnBanIP::cmdName();
    objNames << Cast::cmdName();
    objNames << OpenSubChannel::cmdName();
    objNames << CloseSubChannel::cmdName();
    objNames << LsOpenChannels::cmdName();
    objNames << ListGroups::cmdName();
    objNames << CreateGroup::cmdName();
    objNames << TransGroup::cmdName();
    objNames << ListCommands::cmdName();
    objNames << HostInfo::cmdName();
    objNames << IPHist::cmdName();
    objNames << ListMods::cmdName();
    objNames << DelMod::cmdName();
    objNames << UploadMod::cmdName();
    objNames << ListUsers::cmdName();
    objNames << CreateUser::cmdName();
    objNames << RecoverAcct::cmdName();
    objNames << ResetPwRequest::cmdName();
    objNames << VerifyEmail::cmdName();
    objNames << Resume::cmdName();
    objNames << Pause::cmdName();
    objNames << Term::cmdName();
    objNames << AuthLog::cmdName();
    objNames << LsCmdRanks::cmdName();
    objNames << RemoveCmdRank::cmdName();
    objNames << AssignCmdRank::cmdName();
    objNames << ServSettings::cmdName();
    objNames << LockUser::cmdName();
    objNames << NameChangeRequest::cmdName();
    objNames << PasswordChangeRequest::cmdName();
    objNames << ChangeEmail::cmdName();
    objNames << OverWriteEmail::cmdName();
    objNames << ChangeDispName::cmdName();
    objNames << ChangeUsername::cmdName();
    objNames << ChangePassword::cmdName();
    objNames << RemoveUser::cmdName();
    objNames << ChangeGroup::cmdName();
    objNames << IsEmailVerified::cmdName();
    objNames << SetEmailTemplate::cmdName();
    objNames << PreviewEmail::cmdName();
    objNames << MyInfo::cmdName();
    objNames << DownloadFile::cmdName();
    objNames << UploadFile::cmdName();
    objNames << Delete::cmdName();
    objNames << Copy::cmdName();
    objNames << Move::cmdName();
    objNames << ListFiles::cmdName();
    objNames << FileInfo::cmdName();
    objNames << MakePath::cmdName();
    objNames << ChangeDir::cmdName();
    objNames << ListDBG::cmdName();
    objNames << ListCerts::cmdName();
    objNames << CertInfo::cmdName();
    objNames << AddCert::cmdName();
    objNames << RemoveCert::cmdName();
    objNames << ToPeer::cmdName();
    objNames << LsP2P::cmdName();
    objNames << P2POpen::cmdName();
    objNames << P2PClose::cmdName();
    objNames << P2PRequest::cmdName();
    objNames << PingPeers::cmdName();
    objNames << CreateChannel::cmdName();
    objNames << RemoveChannel::cmdName();
    objNames << RenameChannel::cmdName();
    objNames << SetActiveState::cmdName();
    objNames << CreateSubCh::cmdName();
    objNames << RemoveSubCh::cmdName();
    objNames << RenameSubCh::cmdName();
    objNames << ListChannels::cmdName();
    objNames << ListSubCh::cmdName();
    objNames << SearchChannels::cmdName();
    objNames << InviteToCh::cmdName();
    objNames << DeclineChInvite::cmdName();
    objNames << AcceptChInvite::cmdName();
    objNames << RemoveChMember::cmdName();
    objNames << SetMemberLevel::cmdName();
    objNames << SetSubAcessLevel::cmdName();
    objNames << ListMembers::cmdName();
    objNames << AddRDOnlyFlag::cmdName();
    objNames << RemoveRDOnlyFlag::cmdName();
    objNames << ListRDonlyFlags::cmdName();
    objNames << SetGroupRank::cmdName();
    objNames << CmdInfo::cmdName();
    objNames << OwnerOverride::cmdName();
    objNames << Tree::cmdName();
}

void InternalCommandLoader::loadSettings()
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

bool InternalCommandLoader::exists(const QString &cmdName)
{
    return objNames.contains(cmdName, Qt::CaseInsensitive);
}

QStringList InternalCommandLoader::cmdList()
{
    loadSettings();

    if (emailConfirmation && !exists(VerifyEmail::cmdName()))
    {
        objNames.append(VerifyEmail::cmdName());
    }
    else if (!emailConfirmation && exists(VerifyEmail::cmdName()))
    {
        objNames.removeOne(VerifyEmail::cmdName());
    }

    return objNames;
}

QStringList InternalCommandLoader::pubCmdList()
{
    loadSettings();

    QStringList ret;

    ret << ListCommands::cmdName() << Auth::cmdName() << MyInfo::cmdName();

    if (pubReg)
    {
        ret << CreateUser::cmdName();
    }

    if (passwrdResets)
    {
        ret << ResetPwRequest::cmdName() << RecoverAcct::cmdName();
    }

    return ret;
}

QStringList InternalCommandLoader::rankExemptList()
{
    QStringList ret;

    ret << Auth::cmdName();
    ret << MyInfo::cmdName();
    ret << ChangeDispName::cmdName();
    ret << ChangeUsername::cmdName();
    ret << ChangePassword::cmdName();
    ret << ChangeEmail::cmdName();
    ret << ListCommands::cmdName();
    ret << VerifyEmail::cmdName();
    ret << IsEmailVerified::cmdName();
    ret << Resume::cmdName();
    ret << Pause::cmdName();
    ret << Term::cmdName();
    ret << CmdInfo::cmdName();

    return ret;
}

InternCommand *InternalCommandLoader::cmdObj(const QString &name)
{
    InternCommand *ret = nullptr;

    if (objNames.contains(name, Qt::CaseInsensitive))
    {
        if      (noCaseMatch(name, CloseHost::cmdName()))             ret = new CloseHost(this);
        else if (noCaseMatch(name, RestartHost::cmdName()))           ret = new RestartHost(this);
        else if (noCaseMatch(name, Auth::cmdName()))                  ret = new Auth(this);
        else if (noCaseMatch(name, ListBans::cmdName()))              ret = new ListBans(this);
        else if (noCaseMatch(name, BanIP::cmdName()))                 ret = new BanIP(this);
        else if (noCaseMatch(name, UnBanIP::cmdName()))               ret = new UnBanIP(this);
        else if (noCaseMatch(name, Cast::cmdName()))                  ret = new Cast(this);
        else if (noCaseMatch(name, OpenSubChannel::cmdName()))        ret = new OpenSubChannel(this);
        else if (noCaseMatch(name, CloseSubChannel::cmdName()))       ret = new CloseSubChannel(this);
        else if (noCaseMatch(name, LsOpenChannels::cmdName()))        ret = new LsOpenChannels(this);
        else if (noCaseMatch(name, ListGroups::cmdName()))            ret = new ListGroups(this);
        else if (noCaseMatch(name, CreateGroup::cmdName()))           ret = new CreateGroup(this);
        else if (noCaseMatch(name, TransGroup::cmdName()))            ret = new TransGroup(this);
        else if (noCaseMatch(name, ListCommands::cmdName()))          ret = new ListCommands(this);
        else if (noCaseMatch(name, HostInfo::cmdName()))              ret = new HostInfo(this);
        else if (noCaseMatch(name, IPHist::cmdName()))                ret = new IPHist(this);
        else if (noCaseMatch(name, ListMods::cmdName()))              ret = new ListMods(this);
        else if (noCaseMatch(name, DelMod::cmdName()))                ret = new DelMod(this);
        else if (noCaseMatch(name, UploadMod::cmdName()))             ret = new UploadMod(this);
        else if (noCaseMatch(name, ListUsers::cmdName()))             ret = new ListUsers(this);
        else if (noCaseMatch(name, CreateUser::cmdName()))            ret = new CreateUser(this);
        else if (noCaseMatch(name, RecoverAcct::cmdName()))           ret = new RecoverAcct(this);
        else if (noCaseMatch(name, ResetPwRequest::cmdName()))        ret = new ResetPwRequest(this);
        else if (noCaseMatch(name, VerifyEmail::cmdName()))           ret = new VerifyEmail(this);
        else if (noCaseMatch(name, Resume::cmdName()))                ret = new Resume(this);
        else if (noCaseMatch(name, Pause::cmdName()))                 ret = new Pause(this);
        else if (noCaseMatch(name, Term::cmdName()))                  ret = new Term(this);
        else if (noCaseMatch(name, AuthLog::cmdName()))               ret = new AuthLog(this);
        else if (noCaseMatch(name, LsCmdRanks::cmdName()))            ret = new LsCmdRanks(this);
        else if (noCaseMatch(name, RemoveCmdRank::cmdName()))         ret = new RemoveCmdRank(this);
        else if (noCaseMatch(name, AssignCmdRank::cmdName()))         ret = new AssignCmdRank(this);
        else if (noCaseMatch(name, ServSettings::cmdName()))          ret = new ServSettings(this);
        else if (noCaseMatch(name, LockUser::cmdName()))              ret = new LockUser(this);
        else if (noCaseMatch(name, NameChangeRequest::cmdName()))     ret = new NameChangeRequest(this);
        else if (noCaseMatch(name, PasswordChangeRequest::cmdName())) ret = new PasswordChangeRequest(this);
        else if (noCaseMatch(name, ChangeEmail::cmdName()))           ret = new ChangeEmail(this);
        else if (noCaseMatch(name, OverWriteEmail::cmdName()))        ret = new OverWriteEmail(this);
        else if (noCaseMatch(name, ChangeDispName::cmdName()))        ret = new ChangeDispName(this);
        else if (noCaseMatch(name, ChangeUsername::cmdName()))        ret = new ChangeUsername(this);
        else if (noCaseMatch(name, ChangePassword::cmdName()))        ret = new ChangePassword(this);
        else if (noCaseMatch(name, RemoveUser::cmdName()))            ret = new RemoveUser(this);
        else if (noCaseMatch(name, ChangeGroup::cmdName()))           ret = new ChangeGroup(this);
        else if (noCaseMatch(name, IsEmailVerified::cmdName()))       ret = new IsEmailVerified(this);
        else if (noCaseMatch(name, SetEmailTemplate::cmdName()))      ret = new SetEmailTemplate(this);
        else if (noCaseMatch(name, PreviewEmail::cmdName()))          ret = new PreviewEmail(this);
        else if (noCaseMatch(name, MyInfo::cmdName()))                ret = new MyInfo(this);
        else if (noCaseMatch(name, DownloadFile::cmdName()))          ret = new DownloadFile(this);
        else if (noCaseMatch(name, UploadFile::cmdName()))            ret = new UploadFile(this);
        else if (noCaseMatch(name, Delete::cmdName()))                ret = new Delete(this);
        else if (noCaseMatch(name, Copy::cmdName()))                  ret = new Copy(this);
        else if (noCaseMatch(name, Move::cmdName()))                  ret = new Move(this);
        else if (noCaseMatch(name, ListFiles::cmdName()))             ret = new ListFiles(this);
        else if (noCaseMatch(name, FileInfo::cmdName()))              ret = new FileInfo(this);
        else if (noCaseMatch(name, MakePath::cmdName()))              ret = new MakePath(this);
        else if (noCaseMatch(name, ChangeDir::cmdName()))             ret = new ChangeDir(this);
        else if (noCaseMatch(name, ListDBG::cmdName()))               ret = new ListDBG(this);
        else if (noCaseMatch(name, ListCerts::cmdName()))             ret = new ListCerts(this);
        else if (noCaseMatch(name, CertInfo::cmdName()))              ret = new CertInfo(this);
        else if (noCaseMatch(name, AddCert::cmdName()))               ret = new AddCert(this);
        else if (noCaseMatch(name, RemoveCert::cmdName()))            ret = new RemoveCert(this);
        else if (noCaseMatch(name, ToPeer::cmdName()))                ret = new ToPeer(this);
        else if (noCaseMatch(name, LsP2P::cmdName()))                 ret = new LsP2P(this);
        else if (noCaseMatch(name, P2POpen::cmdName()))               ret = new P2POpen(this);
        else if (noCaseMatch(name, P2PClose::cmdName()))              ret = new P2PClose(this);
        else if (noCaseMatch(name, P2PRequest::cmdName()))            ret = new P2PRequest(this);
        else if (noCaseMatch(name, PingPeers::cmdName()))             ret = new PingPeers(this);
        else if (noCaseMatch(name, CreateChannel::cmdName()))         ret = new CreateChannel(this);
        else if (noCaseMatch(name, RemoveChannel::cmdName()))         ret = new RemoveChannel(this);
        else if (noCaseMatch(name, RenameChannel::cmdName()))         ret = new RenameChannel(this);
        else if (noCaseMatch(name, SetActiveState::cmdName()))        ret = new SetActiveState(this);
        else if (noCaseMatch(name, CreateSubCh::cmdName()))           ret = new CreateSubCh(this);
        else if (noCaseMatch(name, RemoveSubCh::cmdName()))           ret = new RemoveSubCh(this);
        else if (noCaseMatch(name, RenameSubCh::cmdName()))           ret = new RenameSubCh(this);
        else if (noCaseMatch(name, ListChannels::cmdName()))          ret = new ListChannels(this);
        else if (noCaseMatch(name, ListSubCh::cmdName()))             ret = new ListSubCh(this);
        else if (noCaseMatch(name, SearchChannels::cmdName()))        ret = new SearchChannels(this);
        else if (noCaseMatch(name, InviteToCh::cmdName()))            ret = new InviteToCh(this);
        else if (noCaseMatch(name, DeclineChInvite::cmdName()))       ret = new DeclineChInvite(this);
        else if (noCaseMatch(name, AcceptChInvite::cmdName()))        ret = new AcceptChInvite(this);
        else if (noCaseMatch(name, RemoveChMember::cmdName()))        ret = new RemoveChMember(this);
        else if (noCaseMatch(name, SetMemberLevel::cmdName()))        ret = new SetMemberLevel(this);
        else if (noCaseMatch(name, SetSubAcessLevel::cmdName()))      ret = new SetSubAcessLevel(this);
        else if (noCaseMatch(name, ListMembers::cmdName()))           ret = new ListMembers(this);
        else if (noCaseMatch(name, AddRDOnlyFlag::cmdName()))         ret = new AddRDOnlyFlag(this);
        else if (noCaseMatch(name, RemoveRDOnlyFlag::cmdName()))      ret = new RemoveRDOnlyFlag(this);
        else if (noCaseMatch(name, ListRDonlyFlags::cmdName()))       ret = new ListRDonlyFlags(this);
        else if (noCaseMatch(name, SetGroupRank::cmdName()))          ret = new SetGroupRank(this);
        else if (noCaseMatch(name, CmdInfo::cmdName()))               ret = new CmdInfo(this);
        else if (noCaseMatch(name, OwnerOverride::cmdName()))         ret = new OwnerOverride(this);
        else if (noCaseMatch(name, Tree::cmdName()))                  ret = new Tree(this);

        if (ret == nullptr)
        {
            qDebug() << "InternalCommandLoader Error: the loader claims command name '" << name << "' exists but no command object was actually matched/built.";
        }
        else
        {
            ret->setObjectName(name);
            ret->setWritableDataShare(rwShared);

            // This is used as an automated way to update the Internal_Commands doc for the source code documentation
            // keep it commented for release code.

            // appendToDoc("/path/to/source/docs/Internal_Commands.md", name, ret);
        }
    }
    else
    {
        qDebug() << "InternalCommandLoader Error: command name '" << name << "' not found.";
    }

    return ret;
}

void InternalCommandLoader::makeDocHeader(const QString &path)
{
    QFile file(path);

    if (file.open(QFile::WriteOnly | QFile::Text))
    {
        file.write("### 7.1 Internal Commands ###\n\n");
        file.write("The host is extendable via 3rd party modules but the host itself have it's own internal module that load ");
        file.write("commands with direct access to the host database and several internal power functions that external ");
        file.write("commands would otherwise not have direct access to. \n\n");
    }

    file.close();
}

void InternalCommandLoader::appendToDoc(const QString &path, const QString &cmdName, InternCommand *obj)
{
    QFile     file(path);
    QFileInfo info(path);

    if (file.open(QFile::Append | QFile::Text))
    {
        file.write("* [" + cmdName.toUtf8() + "](intern_commands/" + cmdName.toUtf8() + ".md) - " + obj->shortText().toUtf8() + "\n\n");
    }

    file.close();
}
