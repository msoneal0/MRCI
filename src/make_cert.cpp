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

bool genRSAKey(Cert *cert, QTextStream &msg)
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
                else
                {
                    msg << "Failed to assign the generated RSA key to a PKEY object." << endl;
                }
            }
            else
            {
                msg << "Failed to generate the RSA private key." << endl;
            }
        }
        else
        {
            msg << "Failed to initialize a BIGNUM object needed to generate the RSA key." << endl;
        }
    }
    else
    {
        msg << "The x509 object did not initialize correctly." << endl;
    }

    return ret;
}

bool genX509(Cert *cert, const QString &outsideAddr, QTextStream &msg)
{
    auto ret        = false;
    auto interfaces = QNetworkInterface::allAddresses();

    QList<QByteArray> cnNames;

    if (!outsideAddr.isEmpty())
    {
        msg << "x509 gen_wan_ip: " << outsideAddr << endl;

        cnNames.append(outsideAddr.toUtf8());
    }

    for (auto&& addr : interfaces)
    {
        if (addr.isGlobal())
        {
            msg << "x509 gen_lan_ip: " << addr.toString() << endl;

            cnNames.append(addr.toString().toUtf8());
        }
    }

    if (cert->x509 && cert->pKey && !cnNames.isEmpty())
    {
        ASN1_INTEGER_set(X509_get_serialNumber(cert->x509), QDateTime::currentDateTime().toSecsSinceEpoch());

        X509_gmtime_adj(X509_get_notBefore(cert->x509), 0);        // now
        X509_gmtime_adj(X509_get_notAfter(cert->x509), 31536000L); // 365 days
        X509_set_pubkey(cert->x509, cert->pKey);

        // copy the subject name to the issuer name.

        auto *name = X509_get_subject_name(cert->x509);

        X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *) cnNames[0].data(), -1, -1, 0);

        X509_set_issuer_name(cert->x509, name);

        cnNames.removeAt(0);

        QByteArray sanField;

        for (int i = 0; i < cnNames.size(); ++i)
        {
            sanField.append("DNS:" + cnNames[i]);

            if (i != cnNames.size() - 1)
            {
                sanField.append(", ");
            }
        }

        if (!sanField.isEmpty())
        {
            addExt(cert->x509, NID_subject_alt_name, sanField.data());
        }

        if (X509_sign(cert->x509, cert->pKey, EVP_sha1()))
        {
            ret = true;
        }
        else
        {
            msg << "Failed to self-sign the generated x509 cert." << endl;
        }
    }
    else
    {
        msg << "No usable IP addresses could be found to be used as common names in the self-signed cert." << endl;
    }

    return ret;
}

void addExt(X509 *cert, int nid, char *value)
{
    X509_EXTENSION *ext = X509V3_EXT_conf_nid(NULL, NULL, nid, value);

    if (ext != NULL)
    {
        X509_add_ext(cert, ext, -1);
        X509_EXTENSION_free(ext);
    }
}

FILE *openFileForWrite(const char *path, QTextStream &msg)
{
    auto file = fopen(path, "wb");

    if (!file)
    {
        msg << "Cannot open file: '" << path << "' for writing. "  << strerror(errno);
    }

    return file;
}

void encodeErr(const char *path, QTextStream &msg)
{
    msg << "Failed to encode file '" << path << "' to PEM format." << endl;
}

bool writePrivateKey(const char *path, Cert* cert, QTextStream &msg)
{
    auto  ret  = false;
    FILE *file = openFileForWrite(path, msg);

    if (file)
    {
        if (PEM_write_PrivateKey(file, cert->pKey, NULL, NULL, 0, NULL, NULL))
        {
            ret = true;
        }
        else
        {
            encodeErr(path, msg);
        }
    }

    fclose(file);

    return ret;
}

bool writeX509(const char *path, Cert *cert, QTextStream &msg)
{
    auto  ret  = false;
    FILE *file = openFileForWrite(path, msg);

    if (file)
    {
        if (PEM_write_X509(file, cert->x509))
        {
            ret = true;
        }
        else
        {
            encodeErr(path, msg);
        }
    }

    fclose(file);

    return ret;
}

bool genDefaultSSLFiles(const QString &outsideAddr, QTextStream &msg)
{
    auto *cert = new Cert();
    auto  ret  = genRSAKey(cert, msg);

    if (ret) ret = genX509(cert, outsideAddr, msg);
    if (ret) ret = writePrivateKey(DEFAULT_PRIV_KEY_NAME, cert, msg);
    if (ret) ret = writeX509(DEFAULT_PUB_KEY_NAME, cert, msg);

    cert->cleanup();
    cert->deleteLater();

    return ret;
}
