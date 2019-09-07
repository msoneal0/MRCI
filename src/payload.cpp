#include "payload.h"

//    This file is part of MRCI_Host.

//    MRCI_Host is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    MRCI_Host is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with MRCI_Host under the LICENSE.md file. If not, see
//    <http://www.gnu.org/licenses/>.

QByteArray wrInt(quint64 num, int numOfBits)
{
    QByteArray ret(numOfBits / 8, (char) 0);

    num = qToLittleEndian(num);

    memcpy(ret.data(), &num, ret.size());

    return ret;
}

uint rdInt(const QByteArray &bytes)
{
    quint64 ret = 0;

    memcpy(&ret, bytes.data(), bytes.size());

    return qFromLittleEndian(ret);
}

QString sessionCountShareKey()
{
    return QString(APP_NAME) + "::SessionCount";
}

uint rdSessionLoad()
{
    uint ret = 0;

    QSharedMemory mem(sessionCountShareKey());

    if (mem.attach(QSharedMemory::ReadOnly))
    {
        if (mem.size() >= 4)
        {
            mem.lock();

            memcpy(&ret, mem.data(), 4);

            mem.unlock();
        }

        mem.detach();
    }

    return ret;
}

void wrInt(QSharedMemory *mem, uint value)
{
    if (mem->isAttached() && (mem->size() == 4))
    {
        memcpy(mem->data(), &value, 4);
    }
}

QSharedPointer<QByteArray> toIPCPointer(const QByteArray &data)
{
    return QSharedPointer<QByteArray>(new QByteArray(data));
}

QByteArray wrFrame(const QByteArray &data, uchar dType)
{
    return QByteArray(wrInt(dType, 8) + wrInt(data.size(), MAX_BITS) + data);
}

Payload::Payload(QObject *parent) : QObject(parent) {obj = 0;}

InternCommand::InternCommand(QObject *parent) : ExternCommand(parent) {}
