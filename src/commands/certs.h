#ifndef CERTS_H
#define CERTS_H

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

#include "../common.h"
#include "../make_cert.h"
#include "table_viewer.h"

class ListCerts : public TableViewer
{
    Q_OBJECT

public:

    static QString cmdName();

    explicit ListCerts(QObject *parent = nullptr);
};

//--------------------------

class CertInfo : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit CertInfo(QObject *parent = nullptr);
};

//----------------------------

class AddCert : public InternCommand
{
    Q_OBJECT

private:

    QString          coName;
    QByteArray       certBa;
    QByteArray       privBa;
    Query::QueryType qType;

    void run();
    void ask();

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit AddCert(QObject *parent = nullptr);
};

//-----------------------------

class RemoveCert : public InternCommand
{
    Q_OBJECT

private:

    QString coName;

    void run();
    void ask();

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit RemoveCert(QObject *parent = nullptr);
};

#endif // CERTS_H
