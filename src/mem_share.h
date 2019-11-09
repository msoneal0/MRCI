#ifndef MEM_SHARE_H
#define MEM_SHARE_H

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

#include <QObject>
#include <QSharedMemory>
#include <QDebug>
#include <QtEndian>

#define MAX_OPEN_SUB_CHANNELS 6
#define MAX_CHANNELS_PER_USER 200
#define MAX_P2P_LINKS         100

#define BLKSIZE_CLIENT_IP   78
#define BLKSIZE_SESSION_ID  28
#define BLKSIZE_SUB_CHANNEL 9
#define BLKSIZE_APP_NAME    134
#define BLKSIZE_USER_ID     32
#define BLKSIZE_USER_NAME   48
#define BLKSIZE_GROUP_NAME  24
#define BLKSIZE_DISP_NAME   64
#define BLKSIZE_CHANNEL_ID  8
#define BLKSIZE_HOST_RANK   4
#define BLKSIZE_ACT_UPDATE  1
#define BLKSIZE_CH_OVERRIDE 1
#define BLKSIZE_HOST_LOAD   4
#define BLKSIZE_CMD_NAME    128
#define BLKSIZE_LIB_NAME    128
#define BLKSIZE_EMAIL_ADDR  128

#define HOST_NON_NATIVE_KEY "MRCI_Host_Shared_Mem_Key"

int        posOfLikeBlock(const QByteArray &block, const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
int        posOfBlock(const char *block, const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
int        posOfEmptyBlock(const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
int        countNonEmptyBlocks(const char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
bool       rmLikeBlkFromBlkset(const QByteArray &block, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
bool       rmBlockFromBlockset(const char *block, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
bool       addStringToBlockset(const QString &str, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
bool       addBlockToBlockset(const char *block, char *blocks, quint32 numOfBlocks, quint32 bytesPerBlock);
bool       isEmptyBlock(const char *block, quint32 blockSize);
void       wrStringToBlock(const QString &str, char *block, quint32 blockSize);
void       wr8BitToBlock(quint8 num, char *block);
void       wr16BitToBlock(quint16 num, char *block);
void       wr32BitToBlock(quint32 num, char *block);
void       wr64BitToBlock(quint64 num, char *block);
void       wrToBlock(const QByteArray &data, char *block, quint32 blockSize);
QByteArray rdFromBlock(const char *block, quint32 blockSize);
QByteArray wrInt(quint64 num, int numOfBits);
QByteArray wrInt(qint64 num, int numOfBits);
QByteArray wrInt(int num, int numOfBits);
QByteArray wrInt(uint num, int numOfBits);
QString    rdStringFromBlock(const char *block, quint32 blockSize);
QString    createHostSharedMem(QSharedMemory *mem);
quint8     rd8BitFromBlock(const char *block);
quint16    rd16BitFromBlock(const char *block);
quint32    rd32BitFromBlock(const char *block);
quint64    rd64BitFromBlock(const char *block);
quint64    rdInt(const QByteArray &bytes);

class MemShare : public QObject
{
    Q_OBJECT

protected:

    QString        sesMemKey;
    QString        hostMemKey;
    QSharedMemory *hostSharedMem;
    QSharedMemory *sharedMem;
    char          *p2pAccepted;
    char          *p2pPending;
    char          *chList;
    char          *clientIp;
    char          *userName;
    char          *displayName;
    char          *appName;
    char          *openSubChs;
    char          *openWritableSubChs;
    char          *sessionId;
    char          *userId;
    char          *hostRank;
    char          *activeUpdate;
    char          *chOwnerOverride;
    char          *hostLoad;

    bool       createSharedMem(const QByteArray &sesId, const QString &hostKey);
    bool       attachSharedMem(const QString &sKey, const QString &hKey);
    void       setupDataBlocks();
    QByteArray createPeerInfoFrame();

public:

    explicit MemShare(QObject *parent = nullptr);
};

#endif // MEM_SHARE_H
