#include "make_cert.h"

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

Cert::Cert(QObject *parent) : QObject(parent)
{
    pKey = EVP_PKEY_new();
    x509 = X509_new();
    bne  = BN_new();
    rsa  = RSA_new();
}

void Cert::cleanup()
{
    EVP_PKEY_free(pKey);
    X509_free(x509);
    BN_free(bne);
}

QByteArray tempFilePath(const QString &baseName)
{
    return QDir::tempPath().toUtf8() + "/" + baseName.toUtf8() + "_" + QDateTime::currentDateTime().toString("YYYYMMddHHmmsszzz").toUtf8();
}

long genSerialViaDateTime()
{
    QDateTime dateTime  = QDateTime::currentDateTime();
    QString   serialStr = dateTime.toString("YYYYMMddHH");

    return serialStr.toLong();
}

bool genRSAKey(Cert *cert)
{
    bool ret = false;

    if (cert->pKey && cert->bne && cert->rsa)
    {
        if (BN_set_word(cert->bne, RSA_F4))
        {
            if (RSA_generate_key_ex(cert->rsa, 2048, cert->bne, NULL))
            {
                if (EVP_PKEY_assign_RSA(cert->pKey, cert->rsa))
                {
                    ret = true;
                }
            }
        }
    }

    return ret;
}

bool genX509(Cert *cert, const QString &coName)
{
    bool ret = false;

    if (cert->x509 && cert->pKey)
    {
        ASN1_INTEGER_set(X509_get_serialNumber(cert->x509), genSerialViaDateTime());

        X509_gmtime_adj(X509_get_notBefore(cert->x509), 0);        // now
        X509_gmtime_adj(X509_get_notAfter(cert->x509), 31536000L); // 365 days
        X509_set_pubkey(cert->x509, cert->pKey);

        // copy the subject name to the issuer name.

        X509_NAME *name    = X509_get_subject_name(cert->x509);
        QByteArray orgName = QCoreApplication::organizationName().toUtf8();

        X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *) "USA",                  -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *) orgName.data(),         -1, -1, 0);
        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *) coName.toUtf8().data(), -1, -1, 0);

        X509_set_issuer_name(cert->x509, name);

        if (X509_sign(cert->x509, cert->pKey, EVP_sha1()))
        {
            ret = true;
        }
    }

    return ret;
}

bool writePrivateKey(const char *path, Cert* cert)
{
    bool  ret  = false;
    FILE *file = fopen(path, "wb");

    if (file)
    {
        ret = PEM_write_PrivateKey(file, cert->pKey, NULL, NULL, 0, NULL, NULL);
    }

    fclose(file);

    return ret;
}

bool writeX509(const char *path, Cert *cert)
{
    bool  ret  = false;
    FILE *file = fopen(path, "wb");

    if (file)
    {
        ret = PEM_write_X509(file, cert->x509);
    }

    fclose(file);

    return ret;
}

bool getCertAndKey(const QString &coName, QByteArray &cert, QByteArray &privKey)
{
    bool ret = true;

    Query db;

    db.setType(Query::PULL, TABLE_CERT_DATA);
    db.addColumn(COLUMN_CERT);
    db.addColumn(COLUMN_PRIV_KEY);
    db.addCondition(COLUMN_COMMON_NAME, coName);
    db.exec();

    if (db.rows())
    {
        cert    = db.getData(COLUMN_CERT).toByteArray();
        privKey = db.getData(COLUMN_PRIV_KEY).toByteArray();

        QSslCertificate certObj(cert, QSsl::Pem);

        if (certObj.isNull())
        {
            genNewSSLData(coName, cert, privKey, true);

            qDebug() << "The cert for CN: " << coName << " was invalid, generated a new self signed cert.";
        }
        else if (certObj.expiryDate().isValid() && (certObj.expiryDate() < QDateTime::currentDateTime()))
        {
            genNewSSLData(coName, cert, privKey, true);

            qDebug() << "The cert for CN: " << coName << " was expired, generated a new self signed cert.";
        }
    }
    else if (islocalIP(coName) && !coName.isEmpty())
    {
        genNewSSLData(coName, cert, privKey, false);

        qDebug() << "Generated a new self signed cert for CN: " << coName;
    }
    else
    {
        ret = false;
    }

    return ret;
}

void genNewSSLData(const QString &coName, QByteArray &cert, QByteArray &privKey, bool exists)
{
    auto      *newCert     = new Cert();
    QByteArray certPath    = tempFilePath(QString(APP_NAME) + "Cert");
    QByteArray privKeyPath = tempFilePath(QString(APP_NAME) + "PrivKey");

    if (genRSAKey(newCert))
    {
        if (genX509(newCert, coName))
        {
            if (writePrivateKey(privKeyPath.data(), newCert) &&
                writeX509(certPath.data(), newCert))
            {
                QFile certFile(certPath);
                QFile privFile(privKeyPath);

                if (certFile.open(QFile::ReadOnly) && privFile.open(QFile::ReadOnly))
                {
                    Query db;

                    cert    = certFile.readAll();
                    privKey = privFile.readAll();

                    if (exists) db.setType(Query::UPDATE, TABLE_CERT_DATA);
                    else        db.setType(Query::PUSH, TABLE_CERT_DATA);

                    db.addColumn(COLUMN_COMMON_NAME, coName);
                    db.addColumn(COLUMN_CERT, cert);
                    db.addColumn(COLUMN_PRIV_KEY, privKey);
                    db.exec();
                }

                certFile.close();
                privFile.close();
                certFile.remove();
                privFile.remove();
            }
        }
    }

    newCert->cleanup();
    newCert->deleteLater();
}

void genNewSSLData(const QString &coName)
{
    QByteArray cert;
    QByteArray priv;

    genNewSSLData(coName, cert, priv, false);
}

bool islocalIP(const QString &coName)
{
    bool ret = false;

    QList<QHostAddress> interfaces = QNetworkInterface::allAddresses();

    for (auto&& addr : interfaces)
    {
        if ((addr.toString() == coName) && addr.isGlobal())
        {
            qDebug() << "Detected local address: " << addr.toString();

            ret = true;
        }
    }

    return ret;
}

bool certExists(const QString &coName)
{
    Query db;

    db.setType(Query::PULL, TABLE_CERT_DATA);
    db.addColumn(COLUMN_COMMON_NAME);
    db.addCondition(COLUMN_COMMON_NAME, coName);
    db.exec();

    return db.rows();
}

QSslKey toSSLKey(const QByteArray &data)
{
    QSslKey ret(data, QSsl::Rsa, QSsl::Pem);

    if (ret.isNull()) ret = QSslKey(data, QSsl::Dsa, QSsl::Pem);
    if (ret.isNull()) ret = QSslKey(data, QSsl::Ec, QSsl::Pem);
    if (ret.isNull()) ret = QSslKey(data, QSsl::Opaque, QSsl::Pem);

    if (ret.isNull()) ret = QSslKey(data, QSsl::Rsa, QSsl::Der);
    if (ret.isNull()) ret = QSslKey(data, QSsl::Dsa, QSsl::Der);
    if (ret.isNull()) ret = QSslKey(data, QSsl::Ec, QSsl::Der);
    if (ret.isNull()) ret = QSslKey(data, QSsl::Opaque, QSsl::Der);

    return ret;
}

QSslKey toSSLKey(QIODevice *dev)
{
    QSslKey ret(dev, QSsl::Rsa, QSsl::Pem);

    if (ret.isNull()) ret = QSslKey(dev, QSsl::Dsa, QSsl::Pem);
    if (ret.isNull()) ret = QSslKey(dev, QSsl::Ec, QSsl::Pem);
    if (ret.isNull()) ret = QSslKey(dev, QSsl::Opaque, QSsl::Pem);

    if (ret.isNull()) ret = QSslKey(dev, QSsl::Rsa, QSsl::Der);
    if (ret.isNull()) ret = QSslKey(dev, QSsl::Dsa, QSsl::Der);
    if (ret.isNull()) ret = QSslKey(dev, QSsl::Ec, QSsl::Der);
    if (ret.isNull()) ret = QSslKey(dev, QSsl::Opaque, QSsl::Der);

    return ret;
}

QSslCertificate toSSLCert(const QByteArray &data)
{
    QSslCertificate ret(data, QSsl::Pem);

    if (ret.isNull()) ret = QSslCertificate(data, QSsl::Der);

    return ret;
}

QSslCertificate toSSLCert(QIODevice *dev)
{
    QSslCertificate ret(dev, QSsl::Pem);

    if (ret.isNull()) ret = QSslCertificate(dev, QSsl::Der);

    return ret;
}
