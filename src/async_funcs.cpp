#include "session.h"

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

void Session::acctDeleted(const QByteArray &data)
{
    if (flags & LOGGED_IN)
    {
        // format: [32bytes(user_id)]

        if (memcmp(userId, data.data(), BLKSIZE_USER_ID) == 0)
        {
            logout("", true);
            asyncToClient(ASYNC_SYS_MSG, toTEXT("\nsystem: your session was forced to logout because your account was deleted.\n"), TEXT);
            asyncToClient(ASYNC_USER_DELETED, data, TEXT);
        }
    }
}

void Session::acctEdited(const QByteArray &data)
{
    if (flags & LOGGED_IN)
    {
        // format: [32bytes(user_id)]

        if (memcmp(userId, data.data(), BLKSIZE_USER_ID) == 0)
        {
            sendLocalInfo();
        }
    }
}

void Session::acctRenamed(const QByteArray &data)
{
    if (flags & LOGGED_IN)
    {
        // format: [32bytes(user_id)][48bytes(new_user_name)]

        if (memcmp(userId, data.data(), BLKSIZE_USER_ID) == 0)
        {
            memcpy(userName, data.data() + BLKSIZE_USER_ID, BLKSIZE_USER_NAME);
            castPeerInfo(PEER_INFO);
            sendLocalInfo();
        }
    }
}

void Session::acctDispChanged(const QByteArray &data)
{
    if (flags & LOGGED_IN)
    {
        // format: [32bytes(user_id)][64bytes(new_disp_name)]

        if (memcmp(userId, data.data(), BLKSIZE_USER_ID) == 0)
        {
            memcpy(displayName, data.data() + BLKSIZE_USER_ID, BLKSIZE_DISP_NAME);
            castPeerInfo(PEER_INFO);
            sendLocalInfo();
        }
    }
}

void Session::castCatch(const QByteArray &data)
{
    // format: [54bytes(chIds)][1byte(typeId)][rest-of-bytes(payload)]

    if (matchAnyCh(openSubChs, data.data()))
    {
        auto payloadOffs = (MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL) + 1;
        auto typeId      = static_cast<quint8>(data[payloadOffs - 1]);
        auto len         = static_cast<quint32>(data.size() - payloadOffs);
        auto *payload    = data.data() + payloadOffs;

        asyncToClient(ASYNC_CAST, rdFromBlock(payload, len), typeId);
    }
}

void Session::directDataFromPeer(const QByteArray &data)
{
    // format: [28bytes(sessionId)][1byte(typeId)][rest-of-bytes(payload)]

    if (memcmp(sessionId, data.data(), BLKSIZE_SESSION_ID) == 0)
    {
        auto  payloadOffs = BLKSIZE_SESSION_ID + 1;
        auto  typeId      = static_cast<quint8>(data[payloadOffs - 1]);
        auto  len         = static_cast<quint32>(data.size() - payloadOffs);
        auto *payload     = data.data() + payloadOffs;

        asyncToClient(ASYNC_TO_PEER, rdFromBlock(payload, len), typeId);
    }
}

void Session::p2p(const QByteArray &data)
{
    // format: [28bytes(dst_sessionId)][28bytes(src_sessionId)][1byte(typeId)][rest-of-bytes(payload)]

    if (memcmp(sessionId, data.data(), BLKSIZE_SESSION_ID) == 0)
    {
        auto  payloadOffs = (BLKSIZE_SESSION_ID * 2) + 1;
        auto *src         = data.data() + BLKSIZE_SESSION_ID;
        auto *payload     = data.data() + payloadOffs;
        auto  len         = static_cast<quint32>(data.size() - payloadOffs);
        auto  typeId      = static_cast<quint8>(data[payloadOffs - 1]);

        if (typeId == P2P_REQUEST)
        {
            if (posOfBlock(src, p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) == -1)
            {
                if (addBlockToBlockset(src, p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID))
                {
                    asyncToClient(ASYNC_P2P, rdFromBlock(payload, len), P2P_REQUEST);
                }
            }
        }
        else if (typeId == P2P_OPEN)
        {
            if (rmBlockFromBlockset(src, p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID))
            {
                if (addBlockToBlockset(src, p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID))
                {
                    asyncToClient(ASYNC_P2P, rdFromBlock(payload, len), P2P_OPEN);
                }
            }
        }
        else if (typeId == P2P_CLOSE)
        {
            if (rmBlockFromBlockset(src, p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID))
            {
                asyncToClient(ASYNC_P2P, rdFromBlock(payload, len), P2P_CLOSE);
            }
            else if (rmBlockFromBlockset(src, p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID))
            {
                asyncToClient(ASYNC_P2P, rdFromBlock(payload, len), P2P_CLOSE);
            }
        }
        else if (posOfBlock(src, p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) != -1)
        {
            asyncToClient(ASYNC_P2P, rdFromBlock(src, BLKSIZE_SESSION_ID) + rdFromBlock(payload, len), typeId);
        }
    }
}

void Session::closeP2P(const QByteArray &data)
{
    // format: [28bytes(src_sessionId)]

    if (rmBlockFromBlockset(data.data(), p2pAccepted, MAX_P2P_LINKS, BLKSIZE_SESSION_ID) ||
        rmBlockFromBlockset(data.data(), p2pPending, MAX_P2P_LINKS, BLKSIZE_SESSION_ID))
    {
        asyncToClient(ASYNC_P2P, data, P2P_CLOSE);
    }
}

void Session::limitedCastCatch(const QByteArray &data)
{
    // format: [54bytes(chIds)][1byte(typeId)][rest-of-bytes(payload)]

    if (rd8BitFromBlock(activeUpdate) && matchAnyCh(openSubChs, data.data()))
    {
        auto  payloadOffs = (MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL) + 1;
        auto  len         = static_cast<quint32>(data.size() - payloadOffs);
        auto  typeId      = static_cast<quint8>(data[payloadOffs - 1]);
        auto *payload     = data.data() + payloadOffs;

        if (typeId == PING_PEERS)
        {
            // PING_PEERS is formatted exactly like PEER_INFO. it only tells this
            // async command to also send PEER_INFO of this session to the session
            // that requested the ping using ASYNC_TO_PEER.

            auto peerId = rdFromBlock(payload, BLKSIZE_SESSION_ID);
            auto typeId = wrInt(PEER_INFO, 8);
            auto info   = createPeerInfoFrame();

            emit asyncToPeers(ASYNC_TO_PEER, peerId + typeId + info);

            asyncToClient(ASYNC_LIMITED_CAST, rdFromBlock(payload, len), PEER_INFO);
        }
        else
        {
            asyncToClient(ASYNC_LIMITED_CAST, rdFromBlock(payload, len), typeId);
        }
    }
}

void Session::updateRankViaUser(const QByteArray &data)
{
    if (flags & LOGGED_IN)
    {
        // format: [32bytes(userId)][4bytes(newRank)]

        if (memcmp(data.data(), userId, BLKSIZE_USER_ID) == 0)
        {
            wr32BitToBlock(rd32BitFromBlock(data.data() + BLKSIZE_USER_ID), hostRank);
            sendLocalInfo();
            loadCmds();
        }
    }
}

void Session::userAddedToChannel(quint16 cmdId, const QByteArray &data)
{
    // format: [8bytes(chId)][32bytes(userId)]

    if (memcmp(data.data() + BLKSIZE_CHANNEL_ID, userId, BLKSIZE_USER_ID) == 0)
    {
        if ((cmdId == ASYNC_NEW_CH_MEMBER) || (cmdId == ASYNC_INVITE_ACCEPTED))
        {
            addBlockToBlockset(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID);
        }

        asyncToClient(cmdId, data, CH_MEMBER_INFO);
    }
    else if (posOfBlock(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID) != -1)
    {
        asyncToClient(cmdId, data, CH_MEMBER_INFO);
    }
}

void Session::userRemovedFromChannel(const QByteArray &data)
{
    // format: [8bytes(chId)][32bytes(user_id)]

    if (memcmp(data.data() + BLKSIZE_CHANNEL_ID, userId, BLKSIZE_USER_ID) == 0)
    {
        closeByChId(rdFromBlock(data.data(), BLKSIZE_CHANNEL_ID), true);
        rmBlockFromBlockset(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID);
        asyncToClient(ASYNC_RM_CH_MEMBER, data, BYTES);
    }
    else if (posOfBlock(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID) != -1)
    {
        asyncToClient(ASYNC_RM_CH_MEMBER, data, BYTES);
    }
}

void Session::channelDeleted(const QByteArray &data)
{
    // format: [8bytes(chId)]

    if (rmBlockFromBlockset(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID))
    {
        closeByChId(data, false);
        asyncToClient(ASYNC_DEL_CH, data, CH_ID);
    }
    else if (rmLikeBlkFromBlkset(data, openSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL))
    {
        rmLikeBlkFromBlkset(data, openWritableSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL);
        asyncToClient(ASYNC_DEL_CH, data, CH_ID);
    }
}

void Session::channelMemberLevelUpdated(const QByteArray &data)
{
    // format: [8bytes(chId)][32bytes(user_id)]

    if (memcmp(data.data() + BLKSIZE_CHANNEL_ID, userId, BLKSIZE_USER_ID) == 0)
    {
        closeByChId(rdFromBlock(data.data(), BLKSIZE_CHANNEL_ID), true);
        asyncToClient(ASYNC_MEM_LEVEL_CHANGED, data, BYTES);
    }
    else if (posOfBlock(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID) != -1)
    {
        asyncToClient(ASYNC_MEM_LEVEL_CHANGED, data, BYTES);
    }
}

void Session::channelRenamed(const QByteArray &data)
{
    // format: [8bytes(chId)]

    if (posOfBlock(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID) != -1)
    {
        asyncToClient(ASYNC_RENAME_CH, data, BYTES);
    }
}

void Session::channelActiveFlagUpdated(const QByteArray &data)
{
    // format: [9bytes(72bit_sub_id)]

    if (posOfBlock(data.data(), openSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL) != -1)
    {
        containsActiveCh(openSubChs, activeUpdate);
    }
}

void Session::subChannelAdded(quint16 cmdId, const QByteArray &data)
{
    // format: [8bytes(64bit_ch_id)][1byte(8bit_sub_ch_id)]

    if (posOfBlock(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID) != -1)
    {
        asyncToClient(cmdId, data, BYTES);
    }
}

void Session::subChannelUpdated(quint16 cmdId, const QByteArray &data)
{
    // format: [9bytes(72bit_sub_id)]

    if (rmBlockFromBlockset(data.data(), openSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL))
    {
        rmBlockFromBlockset(data.data(), openWritableSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL);
        asyncToClient(cmdId, data, BYTES);
    }
    else if (posOfBlock(data.data(), chList, MAX_CHANNELS_PER_USER, BLKSIZE_CHANNEL_ID) != -1)
    {
        asyncToClient(cmdId, data, BYTES);
    }
}

void Session::addModule(const QByteArray &data)
{
    auto modApp = fromTEXT(data);

    if (!modCmdNames.contains(modApp))
    {
        startModProc(modApp);
    }
}

void Session::rmModule(const QByteArray &data)
{
    auto modApp = fromTEXT(data);

    if (modCmdNames.contains(modApp) && (modApp != QCoreApplication::applicationFilePath()))
    {
        for (auto&& cmdName : modCmdNames[modApp])
        {
            quint16 cmdId16 = cmdRealNames.key(cmdName);

            emit killCmd16(cmdId16);

            cmdRealNames.remove(cmdId16);
            cmdUniqueNames.remove(cmdId16);
            cmdAppById.remove(cmdId16);
            cmdIds.removeOne(cmdId16);
        }

        modCmdNames.remove(modApp);
    }
}

void Session::closeSubChannel(const QByteArray &data)
{
    auto oldSubChs = QByteArray(openSubChs, MAX_OPEN_SUB_CHANNELS * BLKSIZE_SUB_CHANNEL);

    if (rmBlockFromBlockset(data.data(), openSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL))
    {
        rmBlockFromBlockset(data.data(), openWritableSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL);

        if (rd8BitFromBlock(activeUpdate))
        {
            castPeerStat(oldSubChs, false);
            containsActiveCh(openSubChs, activeUpdate);
        }
    }
}

void Session::openSubChannel(const QByteArray &data)
{
    if (addBlockToBlockset(data.data(), openSubChs, MAX_OPEN_SUB_CHANNELS, BLKSIZE_SUB_CHANNEL))
    {
        containsActiveCh(openSubChs, activeUpdate);
        rd8BitFromBlock(activeUpdate);
    }
}
