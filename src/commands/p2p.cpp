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

ToPeer::ToPeer(QObject *parent)         : CmdObject(parent) {}
P2PRequest::P2PRequest(QObject *parent) : CmdObject(parent) {}
P2POpen::P2POpen(QObject *parent)       : CmdObject(parent) {}
P2PClose::P2PClose(QObject *parent)     : CmdObject(parent) {}
LsP2P::LsP2P(QObject *parent)           : CmdObject(parent) {}

QString ToPeer::cmdName()     {return "to_peer";}
QString P2PRequest::cmdName() {return "p2p_request";}
QString P2POpen::cmdName()    {return "p2p_open";}
QString P2PClose::cmdName()   {return "p2p_close";}
QString LsP2P::cmdName()      {return "ls_p2p";}

void ToPeer::procIn(const QByteArray &binIn, quint8 dType)
{
    if (binIn.size() >= BLKSIZE_SESSION_ID)
    {
        errTxt("err: The p2p data does not contain a session id header.\n");
    }
    if (posOfBlock(binIn.data(), p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) == -1)
    {
        errTxt("err: You don't currently have an open p2p connection with the requested peer.\n");
    }
    else
    {
        quint32    len    = static_cast<quint32>(binIn.size());
        QByteArray dst    = rdFromBlock(binIn.data(), BLKSIZE_SESSION_ID);
        QByteArray src    = rdFromBlock(sessionId, BLKSIZE_SESSION_ID);
        QByteArray data   = rdFromBlock(binIn.data() + BLKSIZE_SESSION_ID, len - BLKSIZE_SESSION_ID);
        QByteArray typeBa = wrInt(dType, 8);

        async(ASYNC_P2P, PUB_IPC, dst + src + typeBa + data);
    }
}

void P2PRequest::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == SESSION_ID)
    {
        if (binIn.size() != BLKSIZE_SESSION_ID)
        {
            errTxt("err: The given client session id does not equal " + QString::number(BLKSIZE_SESSION_ID) + " bytes.\n");
        }
        else if (posOfBlock(binIn.data(), p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1)
        {
            errTxt("err: You already have an open p2p connection with the requested peer.\n");
        }
        else if (posOfBlock(binIn.data(), p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1)
        {
            errTxt("err: There is already a pending p2p request for this peer.\n");
        }
        else
        {
            QByteArray dst    = rdFromBlock(binIn.data(), BLKSIZE_SESSION_ID);
            QByteArray src    = rdFromBlock(sessionId, BLKSIZE_SESSION_ID);
            QByteArray typeBa = wrInt(P2P_REQUEST, 8);

            async(ASYNC_P2P, PUB_IPC, dst + src + typeBa + createPeerInfoFrame());
        }
    }
}

void P2POpen::procIn(const QByteArray &binIn, quint8 dType)
{   
    if (dType == SESSION_ID)
    {
        if (binIn.size() != BLKSIZE_SESSION_ID)
        {
            errTxt("err: The given client session id does not equal " + QString::number(BLKSIZE_SESSION_ID) + " bytes.\n");
        }
        else if (posOfBlock(binIn.data(), p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1)
        {
            errTxt("err: You already have an open p2p connection with the requested peer.\n");
        }
        else if (posOfBlock(binIn.data(), p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1)
        {
            errTxt("err: There is no pending p2p request for the given peer.\n");
        }
        else
        {
            QByteArray dst    = rdFromBlock(binIn.data(), BLKSIZE_SESSION_ID);
            QByteArray src    = rdFromBlock(sessionId, BLKSIZE_SESSION_ID);
            QByteArray typeBa = wrInt(P2P_OPEN, 8);

            async(ASYNC_P2P, PUB_IPC, dst + src + typeBa + dst);
        }
    }
}

void P2PClose::procIn(const QByteArray &binIn, quint8 dType)
{
    if (dType == SESSION_ID)
    {
        if (binIn.size() != BLKSIZE_SESSION_ID)
        {
            errTxt("err: The given client session id does not equal " + QString::number(BLKSIZE_SESSION_ID) + " bytes.\n");
        }
        else if ((posOfBlock(binIn.data(), p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1) &&
                 (posOfBlock(binIn.data(), p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1))
        {
            errTxt("err: There is no pending p2p request or p2p connection with the given peer.\n");
        }
        else
        {
            QByteArray dst    = rdFromBlock(binIn.data(), BLKSIZE_SESSION_ID);
            QByteArray src    = rdFromBlock(sessionId, BLKSIZE_SESSION_ID);
            QByteArray typeBa = wrInt(P2P_CLOSE, 8);

            async(P2P_CLOSE, PUB_IPC, dst + src + typeBa + dst);
        }
    }
}

QList<QByteArray> LsP2P::lsBlocks(const char *blocks, int maxBlocks, int sizeOfBlock)
{
    QList<QByteArray> ret;

    QByteArray blank(sizeOfBlock, 0x00);

    for (int i = 0; i < (maxBlocks * sizeOfBlock); i += sizeOfBlock)
    {
        QByteArray block = rdFromBlock(blocks + i, static_cast<quint32>(sizeOfBlock));

        if (block != blank)
        {
            ret.append(block);
        }
    }

    return ret;
}

void LsP2P::procIn(const QByteArray &binIn, quint8 dType)
{
    Q_UNUSED(binIn)

    if (dType == TEXT)
    {
        QList<QByteArray>  peerIds = lsBlocks(p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) +
                                     lsBlocks(p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID);
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

            if (posOfBlock(peerId.data(), p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1)
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
                mainTxt(row[i].leftJustified(justLens[i] + 4, ' '));
            }

            mainTxt("\n");
        }
    }
}
