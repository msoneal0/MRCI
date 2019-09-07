#include "certs.h"

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

ListCerts::ListCerts(QObject *parent) : TableViewer(parent)
{
    setParams(TABLE_CERT_DATA, QStringList() << COLUMN_COMMON_NAME, false);
}

CertInfo::CertInfo(QObject *parent)     : InternCommand(parent) {}
AddCert::AddCert(QObject *parent)       : InternCommand(parent) {}
RemoveCert::RemoveCert(QObject *parent) : InternCommand(parent) {}

QString ListCerts::cmdName()  {return "ls_certs";}
QString CertInfo::cmdName()   {return "cert_info";}
QString AddCert::cmdName()    {return "add_cert";}
QString RemoveCert::cmdName() {return "rm_cert";}

void CertInfo::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args   = parseArgs(binIn, 2);
        QString     coName = getParam("-name", args);

        if (coName.isEmpty())
        {
            errTxt("err: The common name argument (-name) was not found or is empty.\n");
        }
        else if (!validCommonName(coName))
        {
            errTxt("err: Invalid common name. it must be less than 200 chars long and contain no spaces.\n");
        }
        else if (!certExists(coName))
        {
            errTxt("err: A cert with common name: '" + coName + "' does not exists.\n");
        }
        else
        {
            QString     txt;
            QTextStream txtOut(&txt);

            Query db(this);

            db.setType(Query::PULL, TABLE_CERT_DATA);
            db.addColumn(COLUMN_CERT);
            db.addCondition(COLUMN_COMMON_NAME, coName);
            db.exec();

            QSslCertificate cert = toSSLCert(db.getData(COLUMN_CERT).toByteArray());

            txtOut << "Self Signed:    " << boolStr(cert.isSelfSigned()) << endl;
            txtOut << "Black Listed:   " << boolStr(cert.isBlacklisted()) << endl;
            txtOut << "Effective Date: " << cert.effectiveDate().toString("MM/dd/yy") << endl;
            txtOut << "Expiry Date:    " << cert.expiryDate().toString("MM/dd/yy") << endl;

            mainTxt(txt);
        }
    }
}

void AddCert::term()
{
    coName.clear();
    certBa.clear();
    privBa.clear();

    emit enableMoreInput(false);
}

void AddCert::run()
{
    Query db(this);

    db.setType(qType, TABLE_CERT_DATA);
    db.addColumn(COLUMN_COMMON_NAME, coName);
    db.addColumn(COLUMN_CERT, certBa);
    db.addColumn(COLUMN_PRIV_KEY, privBa);
    db.exec();

    term();
}

void AddCert::ask()
{
    emit enableMoreInput(true);

    mainTxt("Common name: '" + coName + "' already exists. do you want to replace it? (y/n): ");
}

void AddCert::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if ((dType == TEXT) && moreInputEnabled())
    {
        QString ans = fromTEXT(binIn);

        if (noCaseMatch("n", ans))
        {
            term();
        }
        else if (noCaseMatch("y", ans))
        {
            run();
        }
        else
        {
            ask();
        }
    }
    else if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 7);
        QString     cert  = getParam("-cert", args);
        QString     priv  = getParam("-priv", args);
        bool        force = argExists("-force", args);

        coName = getParam("-name", args);

        QFile certFile(cert, this);
        QFile privFile(priv, this);

        if (coName.isEmpty())
        {
            errTxt("err: The common name (-name) argument was not found or empty.\n");
        }
        else if (cert.isEmpty())
        {
            errTxt("err: The cert file path (-cert) argument was not found or empty.\n");
        }
        else if (priv.isEmpty())
        {
            errTxt("err: The priv key file path (-priv) argument was not found or empty.\n");
        }
        else if (!certFile.exists())
        {
            errTxt("err: The given cert file: '" + cert + "' does not exists or is not a file.\n");
        }
        else if (!privFile.exists())
        {
            errTxt("err: The given priv key file: '" + priv + "' does not exists or is not a file.\n");
        }
        else if (!validCommonName(coName))
        {
            errTxt("err: The common name must be less than or equal to 200 chars long and contain no spaces.\n");
        }
        else if (!certFile.open(QFile::ReadOnly))
        {
            errTxt("err: Unable to open the cert file: '" + cert + "' for reading. reason: " + certFile.errorString() + "\n");
        }
        else if (!privFile.open(QFile::ReadOnly))
        {
            errTxt("err: Unable to open the priv key: '" + priv + "' for reading. reason: " + privFile.errorString() + "\n");
        }
        else if (toSSLCert(&certFile).isNull())
        {
            errTxt("err: The given cert file is not compatible.\n");
        }
        else if (toSSLKey(&privFile).isNull())
        {
            errTxt("err: The given private key is not compatible.\n");
        }
        else
        {
            certBa = certFile.readAll();
            privBa = privFile.readAll();

            if (certExists(coName))
            {
                qType = Query::UPDATE;

                if (force) run();
                else       ask();
            }
            else
            {
                qType = Query::PUSH;

                run();
            }
        }

        certFile.close();
        privFile.close();
    }
}

void RemoveCert::run()
{
    Query db(this);

    db.setType(Query::DEL, TABLE_CERT_DATA);
    db.addCondition(COLUMN_COMMON_NAME, coName);
    db.exec();

    term();
}

void RemoveCert::term()
{
    emit enableMoreInput(false);

    coName.clear();
}

void RemoveCert::ask()
{
    emit enableMoreInput(true);

    mainTxt("Are you sure you want to remove the cert for common name: " + coName + "? (y/n): ");
}

void RemoveCert::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if ((dType == TEXT) && moreInputEnabled())
    {
        QString ans = fromTEXT(binIn);

        if (noCaseMatch("n", ans))
        {
            term();
        }
        else if (noCaseMatch("y", ans))
        {
            run();
        }
        else
        {
            ask();
        }
    }
    else if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, -1);
        QString     name  = getParam("-name", args);
        bool        force = argExists("-force", args);

        if (name.isEmpty())
        {
            errTxt("err: Common name (-name) argument not found or is empty.\n");
        }
        else if (!validCommonName(name))
        {
            errTxt("err: The common name must be lass than or equal to 200 chars long and contain no spaces.\n");
        }
        else if (!certExists(name))
        {
            errTxt("err: The given common name '" + name + "' does not exists.\n");
        }
        else
        {
            coName = name;

            if (force) run();
            else       ask();
        }
    }
}
