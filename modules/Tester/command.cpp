#include "command.h"

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

SharedObjs::SharedObjs(QObject *parent) : QObject(parent)
{
    p2pAccepted     = nullptr;
    p2pPending      = nullptr;
    chIds           = nullptr;
    wrAbleChIds     = nullptr;
    chList          = nullptr;
    activeUpdate    = nullptr;
    chOwnerOverride = nullptr;
    sessionAddr     = nullptr;
    userName        = nullptr;
    groupName       = nullptr;
    displayName     = nullptr;
    appName         = nullptr;
    clientMajor     = nullptr;
    clientMinor     = nullptr;
    clientPatch     = nullptr;
    sessionId       = nullptr;
    userId          = nullptr;
    moreInputCmds   = nullptr;
    activeLoopCmds  = nullptr;
    pausedCmds      = nullptr;
    hostRank        = nullptr;
    cmdNames        = nullptr;
}

bool ExternCommand::errState()
{
    return errSent;
}

void ExternCommand::mainTxt(const QString &txt)
{
    emit dataToClient(QTextCodec::codecForName(TXT_CODEC)->fromUnicode(txt).mid(2), TEXT);
}

void ExternCommand::errTxt(const QString &txt)
{
    errSent = true;

    emit dataToClient(QTextCodec::codecForName(TXT_CODEC)->fromUnicode(txt).mid(2), ERR);
}

void ExternCommand::privTxt(const QString &txt)
{
    emit dataToClient(QTextCodec::codecForName(TXT_CODEC)->fromUnicode(txt).mid(2), PRIV_TEXT);
}

void ExternCommand::bigTxt(const QString &txt)
{
    emit dataToClient(QTextCodec::codecForName(TXT_CODEC)->fromUnicode(txt).mid(2), BIG_TEXT);
}
