#ifndef MAKE_CERT_H
#define MAKE_CERT_H

//    This file is part of MRCI_Client.

//    MRCI_Client is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    MRCI_Client is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with MRCI_Client under the LICENSE.md file. If not, see
//    <http://www.gnu.org/licenses/>.

#include <cstdio>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <QDateTime>
#include <QCoreApplication>
#include <QIODevice>
#include <QFile>
#include <QSslCertificate>
#include <QList>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QSslKey>

#include "db.h"

class Cert : public QObject
{
    Q_OBJECT

public:

    EVP_PKEY *pKey;
    X509     *x509;
    BIGNUM   *bne;
    RSA      *rsa;

    void cleanup();

    explicit Cert(QObject *parent = nullptr);
};

QByteArray      tempFilePath(const QString &baseName);
QSslKey         toSSLKey(const QByteArray &data);
QSslKey         toSSLKey(QIODevice *dev);
QSslCertificate toSSLCert(const QByteArray &data);
QSslCertificate toSSLCert(QIODevice *dev);
long            genSerialViaDateTime();
bool            genRSAKey(Cert *cert);
bool            genX509(Cert *cert, const QString &coName);
bool            writePrivateKey(const char *path, Cert *cert);
bool            writeX509(const char *path, Cert *cert);
bool            islocalIP(const QString &coName);
bool            certExists(const QString &coName);
bool            getCertAndKey(const QString &coName, QByteArray &cert, QByteArray &privKey);
void            genNewSSLData(const QString &coName, QByteArray &cert, QByteArray &privKey, bool exists);
void            genNewSSLData(const QString &coName);

#endif // MAKE_CERT_H
