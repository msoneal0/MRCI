#include "p2p.h"

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

ToPeer::ToPeer(QObject *parent)         : InternCommand(parent) {}
P2PRequest::P2PRequest(QObject *parent) : InternCommand(parent) {}
P2POpen::P2POpen(QObject *parent)       : InternCommand(parent) {}
P2PClose::P2PClose(QObject *parent)     : InternCommand(parent) {}
LsP2P::LsP2P(QObject *parent)           : InternCommand(parent) {}

QString ToPeer::cmdName()     {return "to_peer";}
QString P2PRequest::cmdName() {return "p2p_request";}
QString P2POpen::cmdName()    {return "p2p_open";}
QString P2PClose::cmdName()   {return "p2p_close";}
QString LsP2P::cmdName()      {return "ls_p2p";}

void ToPeer::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    QByteArray peerId = binIn.left(28);

    if (!sharedObjs->p2pAccepted->contains(peerId))
    {
        errTxt("err: You don't current have an open p2p connection with the requested peer.");
    }
    else
    {
        emit toPeer(peerId, binIn.mid(28), dType);
    }
}

void P2PRequest::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == SESSION_ID)
    {
        if (binIn.size() != 28)
        {
            errTxt("err: The given client session id does not equal 28 bytes.");
        }
        else if (sharedObjs->p2pAccepted->contains(binIn))
        {
            errTxt("err: You already have an open p2p connection with the requested peer.");
        }
        else if (sharedObjs->p2pPending->contains(binIn))
        {
            errTxt("err: There is already a pending p2p request for this peer.");
        }
        else
        {
            emit toPeer(binIn, QByteArray(), P2P_REQUEST);
        }
    }
}

void P2POpen::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{   
    if (dType == SESSION_ID)
    {
        if (binIn.size() != 28)
        {
            errTxt("err: The given client session id does not equal 28 bytes.");
        }
        else if (sharedObjs->p2pAccepted->contains(binIn))
        {
            errTxt("err: You already have an open p2p connection with the requested peer.");
        }
        else if (!sharedObjs->p2pPending->contains(binIn))
        {
            errTxt("err: There is no pending p2p request for the given peer.");
        }
        else
        {
            emit toPeer(binIn, QByteArray(), P2P_OPEN);
        }
    }
}

void P2PClose::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == SESSION_ID)
    {
        if (binIn.size() != 28)
        {
            errTxt("err: The given client session id does not equal 28 bytes.");
        }
        else if (!sharedObjs->p2pAccepted->contains(binIn) && !sharedObjs->p2pPending->contains(binIn))
        {
            errTxt("err: There is no pending p2p request or p2p connection with the given peer.");
        }
        else
        {
            emit toPeer(binIn, QByteArray(), P2P_CLOSE);
        }
    }
}

void LsP2P::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(binIn);

    if (dType == TEXT)
    {
        QList<QByteArray>  peerIds = *sharedObjs->p2pAccepted + *sharedObjs->p2pPending;
        QList<QStringList> tableData;
        QStringList        separators;
        QList<int>         justLens;

        for (int i = 0; i < 2; ++i)
        {
            justLens.append(7);
            separators.append("-------");
        }

        tableData.append(QStringList() << "peer_id" << "pending");
        tableData.append(separators);

        for (auto&& peerId: peerIds)
        {
            QString     pending = "0";
            QStringList columnData;

            if (sharedObjs->p2pPending->contains(peerId))
            {
                pending = "1";
            }

            columnData.append(peerId.toHex());
            columnData.append(pending);

            for (int k = 0; k < justLens.size(); ++k)
            {
                if (justLens[k] < columnData[k].size()) justLens[k] = columnData[k].size();
            }

            tableData.append(columnData);
        }

        mainTxt("\n");

        for (auto&& row : tableData)
        {
            for (int i = 0; i < row.size(); ++i)
            {
                mainTxt(row[i].leftJustified(justLens[i] + 2, ' '));
            }

            mainTxt("\n");
        }
    }
}
