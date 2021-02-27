#include "acct_recovery.h"

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

RecoverAcct::RecoverAcct(QObject *parent)         : CmdObject(parent) {}
IsEmailVerified::IsEmailVerified(QObject *parent) : CmdObject(parent) {}
ResetPwRequest::ResetPwRequest(QObject *parent)   : CmdObject(parent) {}
VerifyEmail::VerifyEmail(QObject *parent)         : CmdObject(parent) {}

QString RecoverAcct::cmdName()     {return "recover_acct";}
QString ResetPwRequest::cmdName()  {return "request_pw_reset";}
QString VerifyEmail::cmdName()     {return "verify_email";}
QString IsEmailVerified::cmdName() {return "is_email_verified";}

void delRecoverPw(const QByteArray &uId)
{
    Query db;

    db.setType(Query::DEL, TABLE_PW_RECOVERY);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    db.setType(Query::UPDATE, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_COUNT, false);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_RECOVER_ATTEMPT, true);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();
}

bool expired(const QByteArray &uId)
{
    auto ret = true;

    Query db;

    db.setType(Query::PULL, TABLE_PW_RECOVERY);
    db.addColumn(COLUMN_TIME);
    db.addCondition(COLUMN_USER_ID, uId);
    db.exec();

    auto expiry = db.getData(COLUMN_TIME).toDateTime().addSecs(3600); // pw datetime + 1hour;

    if (expiry > QDateTime::currentDateTimeUtc())
    {
        ret = false;
    }
    else
    {
        delRecoverPw(uId);
    }

    return ret;
}

void RecoverAcct::addToThreshold()
{
    Query db(this);

    // log recovery attempt failure
    db.setType(Query::PUSH, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_USER_ID, uId);
    db.addColumn(COLUMN_IPADDR, rdStringFromBlock(clientIp, BLKSIZE_CLIENT_IP));
    db.addColumn(COLUMN_AUTH_ATTEMPT, false);
    db.addColumn(COLUMN_RECOVER_ATTEMPT, true);
    db.addColumn(COLUMN_COUNT, true);
    db.addColumn(COLUMN_ACCEPTED, false);
    db.exec();

    auto confObj     = confObject();
    auto maxAttempts = confObj[CONF_AUTO_LOCK_LIM].toInt();

    // pull how many failed recovery attempts were made on this account
    db.setType(Query::PULL, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_IPADDR);
    db.addCondition(COLUMN_USER_ID, uId);
    db.addCondition(COLUMN_RECOVER_ATTEMPT, true);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_ACCEPTED, false);
    db.exec();

    if (db.rows() > maxAttempts)
    {
        delRecoverPw(uId);

        // reset recovery attempts
        db.setType(Query::UPDATE, TABLE_AUTH_LOG);
        db.addColumn(COLUMN_COUNT, false);
        db.addCondition(COLUMN_USER_ID, uId);
        db.addCondition(COLUMN_RECOVER_ATTEMPT, true);
        db.addCondition(COLUMN_ACCEPTED, false);
        db.exec();

        retCode = INVALID_PARAMS;
        flags  &= ~MORE_INPUT;

        errTxt("err: You have exceeded the maximum amount of recovery attempts, recovery password deleted.\n");
    }
    else
    {
        errTxt("err: Access denied.\n\n");
        privTxt("Enter the temporary password (leave blank to cancel): ");
    }
}

void RecoverAcct::procIn(const QByteArray &binIn, quint8 dType)
{
    if ((flags & MORE_INPUT) && (dType == TEXT))
    {
        auto pw = QString::fromUtf8(binIn);

        if (inputOk)
        {
            QString errMsg;

            if (pw.isEmpty())
            {
                retCode = ABORTED;
                flags  &= ~MORE_INPUT;

                mainTxt("\n");
            }
            else if (!acceptablePw(pw, uId, &errMsg))
            {
                errTxt(errMsg + "\n\n");
                privTxt("Enter a new password (leave blank to cancel): ");
            }
            else
            {
                Query db(this);

                // reset recovery attempts
                db.setType(Query::UPDATE, TABLE_AUTH_LOG);
                db.addColumn(COLUMN_COUNT, false);
                db.addCondition(COLUMN_USER_ID, uId);
                db.addCondition(COLUMN_RECOVER_ATTEMPT, true);
                db.addCondition(COLUMN_ACCEPTED, false);
                db.exec();

                // log recovery accepted
                db.setType(Query::PUSH, TABLE_AUTH_LOG);
                db.addColumn(COLUMN_USER_ID, uId);
                db.addColumn(COLUMN_IPADDR, rdStringFromBlock(clientIp, BLKSIZE_CLIENT_IP));
                db.addColumn(COLUMN_AUTH_ATTEMPT, false);
                db.addColumn(COLUMN_RECOVER_ATTEMPT, true);
                db.addColumn(COLUMN_COUNT, false);
                db.addColumn(COLUMN_ACCEPTED, true);
                db.exec();

                updatePassword(uId, pw, TABLE_USERS);
                delRecoverPw(uId);

                flags &= ~MORE_INPUT;

                mainTxt("\n");
            }
        }
        else
        {
            if (pw.isEmpty())
            {
                retCode = ABORTED;
                flags  &= ~MORE_INPUT;

                mainTxt("\n");
            }
            else if (!validPassword(pw))
            {
                errTxt("err: Invalid password.\n");
                addToThreshold();
            }
            else if (!auth(uId, pw, TABLE_PW_RECOVERY))
            {
                addToThreshold();
            }
            else
            {
                privTxt("Enter a new password (leave blank to cancel): ");

                inputOk = true;
            }
        }
    }
    else if (dType == TEXT)
    {
        auto args  = parseArgs(binIn, 2);
        auto email = getParam("-email", args);
        auto name  = getParam("-user", args);

        if (!email.isEmpty() && validEmailAddr(email))
        {
            name = getUserNameForEmail(email);
        }

        retCode = INVALID_PARAMS;

        if (name.isEmpty() || !validUserName(name))
        {
            errTxt("err: The -user or -email argument is empty, not found or invalid.\n");
        }
        else if (!userExists(name, &uId))
        {
            errTxt("err: No such user.\n");
        }
        else if (!recoverPWExists(uId))
        {
            errTxt("err: This account does not have a recovery password.\n");
        }
        else if (expired(uId))
        {
            errTxt("err: The recovery password has expired.\n");
        }
        else
        {
            privTxt("Enter the temporary password (leave blank to cancel): ");

            inputOk = false;
            retCode = NO_ERRORS;
            flags  |= MORE_INPUT;
        }
    }
}

void ResetPwRequest::procIn(const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        auto confObj = confObject();
        auto cmdLine = confObj[CONF_MAIL_CLIENT_CMD].toString();
        auto subject = confObj[CONF_PW_RES_EMAIL_SUBJECT].toString();
        auto msgFile = confObj[CONF_PW_RES_EMAIL_TEMP].toString();
        auto args    = parseArgs(binIn, 2);
        auto email   = getParam("-email", args);
        auto name    = getParam("-user", args);

        QByteArray uId;
        QString    body;

        if (!email.isEmpty() && validEmailAddr(email))
        {
            name = getUserNameForEmail(email);
        }

        retCode = INVALID_PARAMS;

        if (name.isEmpty() || !validUserName(name))
        {
            errTxt("err: The -user or -email argument is empty, not found or invalid.\n");
        }
        else if (!userExists(name, &uId, &email))
        {
            errTxt("err: No such user.\n");
        }
        else if (getEmailParams(cmdLine, msgFile, &body))
        {
            retCode = EXECUTION_FAIL;

            auto pw    = genPw();
            auto date  = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss (t)");
            auto dbrdy = false;
            
            if (recoverPWExists(uId))
            {
                dbrdy = updatePassword(uId, pw, TABLE_PW_RECOVERY);
            }
            else
            {
                dbrdy = createTempPw(uId, pw);
            }

            if (dbrdy)
            {
                body.replace(DATE_SUB, date);
                body.replace(USERNAME_SUB, name);
                body.replace(OTP_SUB, pw);

                cmdLine.replace(TARGET_EMAIL_SUB, email);
                cmdLine.replace(SUBJECT_SUB, "'" + escapeChars(subject, '\\', '\'') + "'");
                cmdLine.replace(MSG_SUB, "'" + escapeChars(body, '\\', '\'') + "'");

                if (runDetachedProc(parseArgs(cmdLine.toUtf8(), -1)))
                {
                    retCode = NO_ERRORS;

                    mainTxt("A temporary password was sent to the email address associated with the account. this password will expire in 1 hour.\n");
                }
            }
        }
    }
}

void VerifyEmail::procIn(const QByteArray &binIn, quint8 dType)
{
    if ((flags & MORE_INPUT) && (dType == TEXT))
    {
        auto txt = QString::fromUtf8(binIn);

        if (txt.isEmpty())
        {
            mainTxt("\n");

            retCode = ABORTED;
            flags  &= ~MORE_INPUT;
        }
        else if (txt == code)
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_EMAIL_VERIFIED, true);
            db.addCondition(COLUMN_USER_ID, uId);
            db.exec();

            async(ASYNC_RW_MY_INFO, uId);

            retCode = NO_ERRORS;
            flags  &= ~MORE_INPUT;

            mainTxt("\n");
        }
        else
        {
            errTxt("err: The code you entered does not match.\n\n");
            privTxt("Please try again: ");
        }
    }
    else if (dType == TEXT)
    {
        uId = rdFromBlock(userId, BLKSIZE_USER_ID);

        auto confObj = confObject();
        auto cmdLine = confObj[CONF_MAIL_CLIENT_CMD].toString();
        auto subject = confObj[CONF_EVERIFY_SUBJECT].toString();
        auto msgFile = confObj[CONF_EVERIFY_TEMP].toString();
        auto email   = getEmailForUser(uId);

        QString body;
        QString err;

        retCode = INVALID_PARAMS;

        if (getEmailParams(cmdLine, msgFile, &body))
        {
            flags |= MORE_INPUT;
            code   = QString::number(QRandomGenerator::global()->bounded(100000, 999999));

            auto uName = rdStringFromBlock(userName, BLKSIZE_USER_NAME);
            auto date  = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss (t)");

            body.replace(DATE_SUB, date);
            body.replace(USERNAME_SUB, uName);
            body.replace(OTP_SUB, code);

            cmdLine.replace(TARGET_EMAIL_SUB, email);
            cmdLine.replace(SUBJECT_SUB, "'" + escapeChars(subject, '\\', '\'') + "'");
            cmdLine.replace(MSG_SUB, "'" + escapeChars(body, '\\', '\'') + "'");

            if (runDetachedProc(parseArgs(cmdLine.toUtf8(), -1)))
            {
                privTxt("A confirmation code was sent to your email address: " + email + "\n\n" + "Please enter that code now or leave blank to cancel: ");
            }
            else
            {
                retCode = EXECUTION_FAIL;
            }
        }
    }
}

void IsEmailVerified::procIn(const QByteArray &binIn, quint8 dType)
{
    Q_UNUSED(binIn)

    if (dType == TEXT)
    {
        auto uId = rdFromBlock(userId, BLKSIZE_USER_ID);

        Query db(this);

        db.setType(Query::PULL, TABLE_USERS);
        db.addColumn(COLUMN_EMAIL_VERIFIED);
        db.addCondition(COLUMN_USER_ID, uId);
        db.exec();

        mainTxt(boolStr(db.getData(COLUMN_EMAIL_VERIFIED).toBool()) + "\n");
    }
}
