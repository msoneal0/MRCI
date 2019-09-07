#ifndef ACCT_RECOVERY_H
#define ACCT_RECOVERY_H

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

enum TemplateType
{
    PW_RESET,
    CONFIRM_EMAIL,
    NONE
};

class RecoverAcct : public InternCommand
{
    Q_OBJECT

private:

    QString uName;
    bool    inputOk;

    void delRecoverPw();
    void addToThreshold(const SharedObjs *sharedObjs);

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit RecoverAcct(QObject *parent = nullptr);
};

//---------------

class ResetPwRequest : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit ResetPwRequest(QObject *parent = nullptr);
};

//----------------

class VerifyEmail : public InternCommand
{
    Q_OBJECT

private:

    QString code;

public:

    static QString cmdName();

    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit VerifyEmail(QObject *parent = nullptr);
};

//----------------

class IsEmailVerified : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit IsEmailVerified(QObject *parent = nullptr);
};

//------------------

class SetEmailTemplate : public InternCommand
{
    Q_OBJECT

private:

    QString      bodyText;
    QString      subject;
    QString      len;
    int          dataSent;
    bool         textFromFile;
    TemplateType eType;

    void proc();

public:

    static QString cmdName();

    bool handlesGenfile();
    void term();
    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit SetEmailTemplate(QObject *parent = nullptr);
};

//-----------------

class PreviewEmail : public InternCommand
{
    Q_OBJECT

public:

    static QString cmdName();

    void procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType);

    explicit PreviewEmail(QObject *parent = nullptr);
};

#endif // ACCT_RECOVERY_H
