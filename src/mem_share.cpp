#include "mem_share.h"

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

QString createHostSharedMem(QSharedMemory *mem)
{
    int     len = 0;
    QString ret;

    len += BLKSIZE_HOST_LOAD; // hostLoad

    mem->setKey(HOST_NON_NATIVE_KEY);

    if (mem->create(len))
    {
        ret = mem->nativeKey();
    }
    else if (mem->attach())
    {
        ret = mem->nativeKey();
    }

    return ret;
}

int posOfLikeBlock(const QByteArray &block, const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    int     ret    = -1;
    quint32 cmpLen = static_cast<quint32>(block.size());

    if (cmpLen > bytesPerBlock)
    {
        cmpLen = bytesPerBlock;
    }

    for (uint i = 0; i < numOfBlocks; i += bytesPerBlock)
    {
        if (memcmp(block.data(), blocks + i, cmpLen) == 0)
        {
            ret = static_cast<int>(i);

            break;
        }
    }

    return ret;
}

int posOfBlock(const char *block, const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    int ret = -1;

    for (uint i = 0; i < numOfBlocks; i += bytesPerBlock)
    {
        if (memcmp(block, blocks + i, bytesPerBlock) == 0)
        {
            ret = static_cast<int>(i);

            break;
        }
    }

    return ret;
}

int countNonEmptyBlocks(const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    int   ret   = 0;
    char *blank = new char[bytesPerBlock];

    memset(blank, 0, bytesPerBlock);

    for (uint i = 0; i < numOfBlocks; i += bytesPerBlock)
    {
        if (memcmp(blank, blocks + i, bytesPerBlock) != 0)
        {
            ret++;
        }
    }

    delete[] blank;
    return   ret;
}

int posOfEmptyBlock(const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    char *blank = new char[bytesPerBlock];

    memset(blank, 0, bytesPerBlock);

    int ret = posOfBlock(blank, blocks, numOfBlocks, bytesPerBlock);

    delete[] blank;
    return   ret;
}

bool addBlockToBlockset(const char *block, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    bool ret = false;

    if (posOfBlock(block, blocks, numOfBlocks, bytesPerBlock) == -1)
    {
        int pos = posOfEmptyBlock(blocks, numOfBlocks, bytesPerBlock);

        if (pos != -1)
        {
            memcpy(blocks + pos, block, bytesPerBlock);

            ret = true;
        }
    }

    return ret;
}

bool addStringToBlockset(const QString &str, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    char *block = new char[bytesPerBlock];

    wrStringToBlock(str, block, bytesPerBlock);

    bool ret = addBlockToBlockset(block, blocks, numOfBlocks, bytesPerBlock);

    delete[] block;
    return ret;
}

bool rmBlockFromBlockset(const char *block, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    bool ret = false;
    int  pos = posOfBlock(block, blocks, numOfBlocks, bytesPerBlock);

    if (pos != -1)
    {
        memset(blocks + pos, 0, bytesPerBlock);

        ret = true;
    }

    return ret;
}

bool rmLikeBlkFromBlkset(const QByteArray &block, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock)
{
    bool ret = false;
    int  pos = posOfLikeBlock(block, blocks, numOfBlocks, bytesPerBlock);

    if (pos != -1)
    {
        memset(blocks + pos, 0, bytesPerBlock);

        ret = true;
    }

    return ret;
}

bool isEmptyBlock(const char *block, quint32 blockSize)
{
    char *blank = new char[blockSize];

    memset(blank, 0, blockSize);

    return memcmp(block, blank, blockSize) == 0;
}

void wrStringToBlock(const QString &str, char *block, quint32 blockSize)
{
    quint32 strByteSize = static_cast<quint32>(str.size()) * 2;

    if (strByteSize > blockSize)
    {
        strByteSize = blockSize;
    }
    else if (strByteSize != blockSize)
    {
        memset(block, 0, blockSize);
    }

    memcpy(block, reinterpret_cast<const char*>(str.utf16()), strByteSize);
}

void wr8BitToBlock(quint8 num, char *block)
{
    memcpy(block, &num, 1);
}

void wr16BitToBlock(quint16 num, char *block)
{
    memcpy(block, &num, 2);
}

void wr32BitToBlock(quint32 num, char *block)
{
    memcpy(block, &num, 4);
}

void wr64BitToBlock(quint64 num, char *block)
{
    memcpy(block, &num, 8);
}

void wrToBlock(const QByteArray &data, char *block, quint32 blockSize)
{
    quint32 actualSize = static_cast<quint32>(data.size());

    if (actualSize > blockSize)
    {
        actualSize = blockSize;
    }
    else if (actualSize != blockSize)
    {
        memset(block, 0, blockSize);
    }

    memcpy(block, data.data(), actualSize);
}

QByteArray wrInt(quint64 num, int numOfBits)
{
    QByteArray ret(numOfBits / 8, static_cast<char>(0));

    num = qToLittleEndian(num);

    memcpy(ret.data(), &num, static_cast<size_t>(ret.size()));

    return ret;
}

QByteArray wrInt(qint64 num, int numOfBits)
{
    return wrInt(static_cast<quint64>(num), numOfBits);
}

QByteArray wrInt(int num, int numOfBits)
{
    return wrInt(static_cast<quint64>(num), numOfBits);
}

QByteArray wrInt(uint num, int numOfBits)
{
    return wrInt(static_cast<quint64>(num), numOfBits);
}

quint8 rd8BitFromBlock(const char *block)
{
    return static_cast<quint8>(block[0]);
}

quint16 rd16BitFromBlock(const char *block)
{
    quint16 ret;

    memcpy(&ret, block, 2);

    return ret;
}

quint32 rd32BitFromBlock(const char *block)
{
    quint32 ret;

    memcpy(&ret, block, 4);

    return ret;
}

quint64 rd64BitFromBlock(const char *block)
{
    quint64 ret;

    memcpy(&ret, block, 8);

    return ret;
}

QString rdStringFromBlock(const char *block, quint32 blockSize)
{
    QString ret;
    quint16 chr;

    for (quint32 i = 0; i < blockSize; i += 2)
    {
        memcpy(&chr, block + i, 2);

        if (chr == 0)
        {
            break;
        }
        else
        {
            ret.append(QChar(chr));
        }
    }

    return ret;
}

QByteArray rdFromBlock(const char *block, quint32 blockSize)
{
    if (blockSize == 0)
    {
        return QByteArray();
    }
    else
    {
        return QByteArray::fromRawData(block, static_cast<int>(blockSize));
    }
}

quint64 rdInt(const QByteArray &bytes)
{
    quint64 ret = 0;

    memcpy(&ret, bytes.data(), static_cast<size_t>(bytes.size()));

    return qFromLittleEndian(ret);
}

MemShare::MemShare(QObject *parent) : QObject(parent)
{
    sharedMem     = new QSharedMemory(this);
    hostSharedMem = new QSharedMemory(this);
}

bool MemShare::createSharedMem(const QByteArray &sesId, const QString &hostKey)
{
    int  len = 0;
    bool ret = false;

    sharedMem->setKey(sesId.toHex());
    hostSharedMem->setNativeKey(hostKey);

    len += BLKSIZE_SESSION_ID;                            // sessionId
    len += BLKSIZE_USER_ID;                               // userId
    len += BLKSIZE_CLIENT_IP;                             // clientIp
    len += BLKSIZE_APP_NAME;                              // appName
    len += BLKSIZE_USER_NAME;                             // userName
    len += BLKSIZE_DISP_NAME;                             // displayName
    len += BLKSIZE_HOST_RANK;                             // hostRank
    len += BLKSIZE_ACT_UPDATE;                            // activeUpdate
    len += BLKSIZE_CH_OVERRIDE;                           // chOwnerOverride
    len += (BLKSIZE_CHANNEL_ID * MAX_CHANNELS_PER_USER);  // chList
    len += (BLKSIZE_SUB_CHANNEL * MAX_OPEN_SUB_CHANNELS); // openSubChs
    len += (BLKSIZE_SUB_CHANNEL * MAX_OPEN_SUB_CHANNELS); // openWritableSubChs
    len += (BLKSIZE_SESSION_ID * MAX_P2P_LINKS);          // p2pPending
    len += (BLKSIZE_SESSION_ID * MAX_P2P_LINKS);          // p2pAccepted

    if (!sharedMem->create(len))
    {
        qDebug() << "err: Failed to create a shared memory master block. reason: " + sharedMem->errorString();
    }
    else if (!hostSharedMem->attach())
    {
        qDebug() << "err: Failed to attach to the host shared memory master block. reason: " + hostSharedMem->errorString();
    }
    else
    {
        memset(sharedMem->data(), 0, static_cast<quint32>(len));
        memcpy(sharedMem->data(), sesId.data(), BLKSIZE_SESSION_ID);

        ret = true;
    }

    return ret;
}

bool MemShare::attachSharedMem(const QString &sKey, const QString &hKey)
{
    bool ret = false;

    sharedMem->setNativeKey(sKey);
    hostSharedMem->setNativeKey(hKey);

    if (!sharedMem->attach())
    {
        qDebug() << "err: Failed to attach to the session shared memory master block. reason: " + sharedMem->errorString();
    }
    else if (!hostSharedMem->attach())
    {
        qDebug() << "err: Failed to attach to the host shared memory master block. reason: " + hostSharedMem->errorString();
    }
    else
    {
        ret = true;
    }

    return ret;
}

void MemShare::setupDataBlocks()
{
    if (sharedMem->isAttached() && hostSharedMem->isAttached())
    {
        char *sesMasterBlock = static_cast<char*>(sharedMem->data());
        char *hosMasterBlock = static_cast<char*>(hostSharedMem->data());
        int   sesOffs        = 0;
        int   hosOffs        = 0;

        sessionId          = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_SESSION_ID;
        userId             = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_USER_ID;
        clientIp           = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_CLIENT_IP;
        appName            = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_APP_NAME;
        userName           = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_USER_NAME;
        displayName        = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_DISP_NAME;
        hostRank           = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_HOST_RANK;
        activeUpdate       = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_ACT_UPDATE;
        chOwnerOverride    = sesMasterBlock + sesOffs; sesOffs += BLKSIZE_CH_OVERRIDE;
        chList             = sesMasterBlock + sesOffs; sesOffs += (BLKSIZE_CHANNEL_ID * MAX_CHANNELS_PER_USER);
        openSubChs         = sesMasterBlock + sesOffs; sesOffs += (BLKSIZE_SUB_CHANNEL * MAX_OPEN_SUB_CHANNELS);
        openWritableSubChs = sesMasterBlock + sesOffs; sesOffs += (BLKSIZE_SUB_CHANNEL * MAX_OPEN_SUB_CHANNELS);
        p2pPending         = sesMasterBlock + sesOffs; sesOffs += (BLKSIZE_SESSION_ID * MAX_P2P_LINKS);
        p2pAccepted        = sesMasterBlock + sesOffs; sesOffs += (BLKSIZE_SESSION_ID * MAX_P2P_LINKS);
        hostLoad           = hosMasterBlock + hosOffs; hosOffs += BLKSIZE_HOST_LOAD;
        sesMemKey          = sharedMem->nativeKey();
        hostMemKey         = hostSharedMem->nativeKey();
    }
}

QByteArray MemShare::createPeerInfoFrame()
{
    QByteArray sesId = rdFromBlock(sessionId, BLKSIZE_SESSION_ID);
    QByteArray usrId = rdFromBlock(userId, BLKSIZE_USER_ID);
    QByteArray uName = rdFromBlock(userName, BLKSIZE_USER_NAME);
    QByteArray aName = rdFromBlock(appName, BLKSIZE_APP_NAME);
    QByteArray dName = rdFromBlock(displayName, BLKSIZE_DISP_NAME);

    return sesId + usrId + uName + aName + dName;
}
