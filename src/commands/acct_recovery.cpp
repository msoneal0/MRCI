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

RecoverAcct::RecoverAcct(QObject *parent)           : InternCommand(parent) {inputOk = false;}
ResetPwRequest::ResetPwRequest(QObject *parent)     : InternCommand(parent) {}
VerifyEmail::VerifyEmail(QObject *parent)           : InternCommand(parent) {}
IsEmailVerified::IsEmailVerified(QObject *parent)   : InternCommand(parent) {}
SetEmailTemplate::SetEmailTemplate(QObject *parent) : InternCommand(parent) {}
PreviewEmail::PreviewEmail(QObject *parent)         : InternCommand(parent) {}

QString RecoverAcct::cmdName()      {return "recover_acct";}
QString ResetPwRequest::cmdName()   {return "request_pw_reset";}
QString VerifyEmail::cmdName()      {return "verify_email";}
QString IsEmailVerified::cmdName()  {return "is_email_verified";}
QString SetEmailTemplate::cmdName() {return "set_email_template";}
QString PreviewEmail::cmdName()     {return "preview_email";}

void RecoverAcct::term()
{
    emit enableMoreInput(false);

    inputOk = false;
}

void RecoverAcct::delRecoverPw()
{
    Query db(this);

    db.setType(Query::DEL, TABLE_PW_RECOVERY);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    db.setType(Query::UPDATE, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_COUNT, false);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_RECOVER_ATTEMPT, true);
    db.addCondition(COLUMN_USERNAME, uName);
    db.exec();

    term();
}

void RecoverAcct::addToThreshold(const SharedObjs *sharedObjs)
{
    Query db(this);

    db.setType(Query::PUSH, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_USERNAME, uName);
    db.addColumn(COLUMN_IPADDR, *sharedObjs->sessionAddr);
    db.addColumn(COLUMN_AUTH_ATTEMPT, false);
    db.addColumn(COLUMN_RECOVER_ATTEMPT, true);
    db.addColumn(COLUMN_COUNT, true);
    db.addColumn(COLUMN_ACCEPTED, false);
    db.exec();

    db.setType(Query::PULL, TABLE_SERV_SETTINGS);
    db.addColumn(COLUMN_LOCK_LIMIT);
    db.exec();

    uint maxAttempts = db.getData(COLUMN_LOCK_LIMIT).toUInt();

    db.setType(Query::PULL, TABLE_AUTH_LOG);
    db.addColumn(COLUMN_IPADDR);
    db.addCondition(COLUMN_USERNAME, uName);
    db.addCondition(COLUMN_RECOVER_ATTEMPT, true);
    db.addCondition(COLUMN_COUNT, true);
    db.addCondition(COLUMN_ACCEPTED, false);
    db.exec();

    if (static_cast<uint>(db.rows()) > maxAttempts)
    {
        delRecoverPw();
        term();
    }
    else
    {
        errTxt("err: Access denied.\n");
        privTxt("Enter the temporary password: ");
    }
}

void RecoverAcct::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (moreInputEnabled() && (dType == TEXT))
    {
        QString pw = fromTEXT(binIn);

        if (inputOk)
        {
            if (!validPassword(pw))
            {
                errTxt("err: Invalid password. it must be 8-200 chars long containing numbers, mixed case letters and special chars.\n");
                privTxt("Enter a new password: ");
            }
            else
            {
                updatePassword(uName, pw, TABLE_USERS);
                delRecoverPw();
                term();
            }
        }
        else
        {
            if (pw.isEmpty())
            {
                term();
            }
            else if (!validPassword(pw))
            {
                addToThreshold(sharedObjs);
            }
            else if (!auth(uName, pw, TABLE_PW_RECOVERY))
            {
                addToThreshold(sharedObjs);
            }
            else
            {
                privTxt("Enter a new password: ");

                inputOk = true;
            }
        }
    }
    else if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 2);
        QString     email = getParam("-email", args);
        QString     name  = getParam("-user", args);

        if (!email.isEmpty() && validEmailAddr(email)) name = getUserNameForEmail(email);

        if (name.isEmpty() || !validUserName(name))
        {
            errTxt("err: The -user or -email argument is empty, not found or invalid.\n");
        }
        else if (!userExists(name))
        {
            errTxt("err: No such user.\n");
        }
        else if (!recoverPWExists(name))
        {
            errTxt("err: This account does not have a recovery password.\n");
        }
        else
        {
            privTxt("Enter the temporary password (leave blank to cancel): ");

            uName = name;

            emit enableMoreInput(true);
        }
    }
}

void ResetPwRequest::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (dType == TEXT)
    {
        QStringList args  = parseArgs(binIn, 2);
        QString     email = getParam("-email", args);
        QString     name  = getParam("-user", args);

        if (!email.isEmpty() && validEmailAddr(email)) name = getUserNameForEmail(email);

        if (name.isEmpty() || !validUserName(name))
        {
            errTxt("err: The -user or -email argument is empty, not found or invalid.\n");
        }
        else if (!userExists(name))
        {
            errTxt("err: No such user.\n");
        }
        else
        {
            email = getEmailForUser(name);

            QString pw   = genPw();
            QString date = QDateTime::currentDateTimeUtc().toString("YYYY-MM-DD HH:MM:SS");

            if (recoverPWExists(name))
            {
                updatePassword(name, pw, TABLE_PW_RECOVERY);
            }
            else
            {
                createTempPw(name, email, pw);
            }

            Query db(this);

            db.setType(Query::PULL, TABLE_SERV_SETTINGS);
            db.addColumn(COLUMN_TEMP_PW_SUBJECT);
            db.addColumn(COLUMN_TEMP_PW_MSG);
            db.addColumn(COLUMN_MAILERBIN);
            db.addColumn(COLUMN_MAIL_SEND);
            db.exec();

            QString subject = db.getData(COLUMN_TEMP_PW_SUBJECT).toString();
            QString body    = db.getData(COLUMN_TEMP_PW_MSG).toString();
            QString app     = db.getData(COLUMN_MAILERBIN).toString();
            QString cmdLine = db.getData(COLUMN_MAIL_SEND).toString();

            body.replace(DATE_SUB, date);
            body.replace(USERNAME_SUB, name);
            body.replace(TEMP_PW_SUB, pw);

            cmdLine.replace(TARGET_EMAIL_SUB, email);
            cmdLine.replace(SUBJECT_SUB, "'" + escapeChars(subject, '\\', '\'') + "'");
            cmdLine.replace(MSG_SUB, "'" + escapeChars(body, '\\', '\'') + "'");

            QProcess::startDetached(expandEnvVariables(app), parseArgs(toTEXT(cmdLine), -1));

            mainTxt("A temporary password was sent to the email address associated with the account.\n");
        }
    }
}

void VerifyEmail::term()
{
    emit enableMoreInput(false);

    code.clear();
}

void VerifyEmail::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (moreInputEnabled() && (dType == TEXT))
    {
        QString txt = fromTEXT(binIn);

        if (txt == code)
        {
            Query db(this);

            db.setType(Query::UPDATE, TABLE_USERS);
            db.addColumn(COLUMN_EMAIL_VERIFIED, true);
            db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
            db.exec();

            emit backendDataOut(ASYNC_RW_MY_INFO, toTEXT(*sharedObjs->userName), PUB_IPC_WITH_FEEDBACK);

            term();
        }
        else if (txt.isEmpty())
        {
            term();
        }
        else
        {
            errTxt("err: The code you entered does not match.\n");
            privTxt("Please try again: ");
        }
    }
    else if (dType == TEXT)
    {
        QString email = getEmailForUser(*sharedObjs->userName);

        if (email.isEmpty())
        {
            errTxt("err: Your account currently has no email address, please update it.\n");
        }
        else
        {
            QString date = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");

            code = QString::number(QRandomGenerator::global()->bounded(100000, 999999));

            Query db(this);

            db.setType(Query::PULL, TABLE_SERV_SETTINGS);
            db.addColumn(COLUMN_CONFIRM_SUBJECT);
            db.addColumn(COLUMN_CONFIRM_MSG);
            db.addColumn(COLUMN_MAILERBIN);
            db.addColumn(COLUMN_MAIL_SEND);
            db.exec();

            QString subject = db.getData(COLUMN_CONFIRM_SUBJECT).toString();
            QString body    = db.getData(COLUMN_CONFIRM_MSG).toString();
            QString app     = db.getData(COLUMN_MAILERBIN).toString();
            QString cmdLine = db.getData(COLUMN_MAIL_SEND).toString();

            body.replace(DATE_SUB, date);
            body.replace(USERNAME_SUB, *sharedObjs->userName);
            body.replace(CONFIRM_CODE_SUB, code);

            cmdLine.replace(TARGET_EMAIL_SUB, email);
            cmdLine.replace(SUBJECT_SUB, "'" + escapeChars(subject, '\\', '\'') + "'");
            cmdLine.replace(MSG_SUB, "'" + escapeChars(body, '\\', '\'') + "'");

            QProcess::startDetached(expandEnvVariables(app), parseArgs(toTEXT(cmdLine), -1));

            privTxt("A confirmation code was sent to your email address: " + email + "\n\n" + "Please enter that code now or leave blank to cancel: ");

            emit enableMoreInput(true);
        }
    }
}

void IsEmailVerified::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(binIn);

    if (dType == TEXT)
    {
        Query db(this);

        db.setType(Query::PULL, TABLE_USERS);
        db.addColumn(COLUMN_EMAIL_VERIFIED);
        db.addCondition(COLUMN_USERNAME, *sharedObjs->userName);
        db.exec();

        mainTxt(boolStr(db.getData(COLUMN_EMAIL_VERIFIED).toBool()) + "\n");
    }
}

bool SetEmailTemplate::handlesGenfile()
{
    return true;
}

void SetEmailTemplate::term()
{
    emit enableMoreInput(false);

    textFromFile = false;
    dataSent     = 0;

    subject.clear();
    bodyText.clear();
    len.clear();
}

void SetEmailTemplate::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    Q_UNUSED(sharedObjs);

    if (moreInputEnabled() && (dType == GEN_FILE))
    {
        bodyText.append(fromTEXT(binIn));

        dataSent += binIn.size();

        mainTxt(QString::number(dataSent) + "/" + len + "\n");

        if (dataSent >= len.toInt())
        {
            emit enableMoreInput(false);

            mainTxt("\nUpload complete.\n");
            proc();
        }
    }
    else if (dType == GEN_FILE)
    {
        QStringList args = parseArgs(binIn, 9);

        textFromFile = argExists("-client_file", args);
        subject      = getParam("-subject", args);
        bodyText     = getParam("-body", args);
        len          = getParam("-len", args);

        if (argExists("-reset_template", args))
        {
            eType = PW_RESET;
        }
        else if (argExists("-confirm_template", args))
        {
            eType = CONFIRM_EMAIL;
        }
        else
        {
            eType = NONE;
        }

        if (eType == NONE)
        {
            errTxt("err: Which template do you want to change? -reset_template or -confirm_template not found.\n");
            term();
        }
        else if (textFromFile && !isInt(len))
        {
            errTxt("err: '" + len + "' given in -len is not a valid integer.\n");
            term();
        }
        else if (textFromFile && (len.toInt() <= 0))
        {
            errTxt("err: The text file size cannot be 0 or less than 0.\n");
            term();
        }
        else if (textFromFile && (len.toInt() > 20000))
        {
            errTxt("err: The text file size is too large. it cannot exceed 20,000 bytes or 10,000 chars.\n");
            term();
        }
        else
        {
            if (argExists("-subject", args) && subject.isEmpty())
            {
                if (eType == CONFIRM_EMAIL) subject = DEFAULT_CONFIRM_SUBJECT;
                else                        subject = DEFAULT_TEMP_PW_SUBJECT;
            }

            if (argExists("-body", args) && bodyText.isEmpty())
            {
                if (eType == CONFIRM_EMAIL) bodyText = TXT_ConfirmCodeTemplate;
                else                        bodyText = TXT_TempPwTemplate;
            }

            if (textFromFile)
            {
                mainTxt("Input hooked...awaiting text file data.\n\n");

                bodyText.clear();

                emit enableMoreInput(true);
                emit dataToClient(toTEXT("-to_host"), GEN_FILE);
                emit dataToClient(QByteArray(), GEN_FILE);
            }
            else
            {
                proc();
            }
        }
    }
}

void SetEmailTemplate::proc()
{
    if (bodyText.isEmpty() && subject.isEmpty())
    {
        errTxt("err: The email body and subject text are empty, nothing will be changed.\n");
    }
    else
    {
        Query db(this);

        db.setType(Query::UPDATE, TABLE_SERV_SETTINGS);

        QString codeSub;
        QString bodyColumn;
        QString subjectColumn;
        bool    execQuery = false;

        if (eType == PW_RESET)
        {
            codeSub       = TEMP_PW_SUB;
            bodyColumn    = COLUMN_TEMP_PW_MSG;
            subjectColumn = COLUMN_TEMP_PW_SUBJECT;
        }
        else
        {
            codeSub       = CONFIRM_CODE_SUB;
            bodyColumn    = COLUMN_CONFIRM_MSG;
            subjectColumn = COLUMN_CONFIRM_SUBJECT;
        }

        if (!bodyText.isEmpty())
        {
            if (!bodyText.contains(DATE_SUB, Qt::CaseInsensitive))
            {
                errTxt("err: The email body does not contain: " + QString(DATE_SUB) + "\n");
            }
            else if (!bodyText.contains(USERNAME_SUB, Qt::CaseInsensitive))
            {
                errTxt("err: The email body does not contain: " + QString(USERNAME_SUB) + "\n");
            }
            else if (!bodyText.contains(codeSub, Qt::CaseInsensitive))
            {
                errTxt("err: The email body does not contain: " + codeSub + "\n");
            }
            else if (bodyText.size() > 10000)
            {
                errTxt("err: The email body is too large. it cannot exceed 10,000 chars.\n");
            }
            else
            {
                mainTxt("Email body updated.\n");

                db.addColumn(bodyColumn, bodyText);

                execQuery = true;
            }
        }

        if (!subject.isEmpty())
        {
            if (subject.size() > 120)
            {
                errTxt("err: The subject is too large. it cannot exceed 120 chars.\n");
            }
            else
            {
                mainTxt("Email subject updated.\n");

                db.addColumn(subjectColumn, subject);

                execQuery = true;
            }
        }

        if (execQuery) db.exec();
    }

    term();
}

void PreviewEmail::procBin(const SharedObjs *sharedObjs, const QByteArray &binIn, uchar dType)
{
    if (dType == TEXT)
    {
        QStringList  args  = parseArgs(binIn, 4);
        TemplateType eType = NONE;

        if      (argExists("-reset_email", args))   eType = PW_RESET;
        else if (argExists("-confirm_email", args)) eType = CONFIRM_EMAIL;

        if (eType == NONE)
        {
            errTxt("err: which template do you want to preview? -reset_email or -confirm_email not found.\n");
        }
        else
        {
            QString codeSub;
            QString code;
            QString bodyColumn;
            QString subjectColumn;
            QString date = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd HH:mm:ss");

            if (eType == PW_RESET)
            {
                codeSub       = TEMP_PW_SUB;
                code          = genPw();
                bodyColumn    = COLUMN_TEMP_PW_MSG;
                subjectColumn = COLUMN_TEMP_PW_SUBJECT;
            }
            else
            {
                codeSub       = CONFIRM_CODE_SUB;
                code          = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
                bodyColumn    = COLUMN_CONFIRM_MSG;
                subjectColumn = COLUMN_CONFIRM_SUBJECT;
            }

            Query db(this);

            db.setType(Query::PULL, TABLE_SERV_SETTINGS);
            db.addColumn(bodyColumn);
            db.addColumn(subjectColumn);
            db.exec();

            QString subject = db.getData(subjectColumn).toString();
            QString body    = db.getData(bodyColumn).toString();

            body.replace(DATE_SUB, date);
            body.replace(USERNAME_SUB, *sharedObjs->userName);
            body.replace(codeSub, code);

            QString     txt;
            QTextStream txtOut(&txt);

            txtOut << "-----Subject-------" << endl << endl;
            txtOut << subject << endl << endl;
            txtOut << "-----Body----------" << endl << endl;

            mainTxt(txt);
            bigTxt(body);
        }
    }
}
