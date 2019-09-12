#include "main.h"

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

QString libName()
{
    return QString(LIB_NAME) + "_" + QString(LIB_VERSION);
}

Loader::Loader(QObject *parent) : CommandLoader(parent)
{
}

bool Loader::hostRevOk(quint64 minRev)
{
    return minRev >= IMPORT_REV;
}

quint64 Loader::rev()
{
    return IMPORT_REV;
}

QStringList Loader::cmdList()
{
    return QStringList() << "test_text" << "test_input" << "test_loop" << "test_inherit";
}

ExternCommand *Loader::cmdObj(const QString &name)
{
    ExternCommand *ret = nullptr;

    if      (name == "test_text")    ret = new ModText(this);
    else if (name == "test_input")   ret = new ModInput(this);
    else if (name == "test_loop")    ret = new ModLoop(this);
    else if (name == "test_inherit") ret = new ModInherit(this);

    return ret;
}

ModText::ModText(QObject *parent) : ExternCommand(parent) {}

QString ModText::shortText() {return "test module text output.";}
QString ModText::ioText()    {return "[none]/[text]";}
QString ModText::longText()  {return "this test the module interface text output. input data is ignored.";}
QString ModText::libText()   {return libName();}

ModInput::ModInput(QObject *parent) : ExternCommand(parent) {}

QString ModInput::shortText() {return "test module input hook.";}
QString ModInput::ioText()    {return "[text]/[text]";}
QString ModInput::longText()  {return "this command will ask you to enter Yes or No and will not release until a valid response is entered. this demonstrates how to impliment a confirmation question using the more input mode.";}
QString ModInput::libText()   {return libName();}

ModLoop::ModLoop(QObject *parent) : ExternCommand(parent) {index = 0;}

QString ModLoop::shortText() {return "test module looping command.";}
QString ModLoop::ioText()    {return "[none]/[text]";}
QString ModLoop::longText()  {return "this command will display 'loop' along with the loop number 10 times to demonstrate looping.";}
QString ModLoop::libText()   {return libName();}

ModInherit::ModInherit(QObject *parent) : ExternCommand(parent) {}

QString ModInherit::shortText() {return "module internal command inheritance test.";}
QString ModInherit::ioText()    {return "[text]/[text]";}
QString ModInherit::longText()  {return "this command will run the output of the known internal command 'my_info' to demonstrate internal command inheritance.";}
QString ModInherit::libText()   {return libName();}

void ModText::procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType)
{
    Q_UNUSED(data)
    Q_UNUSED(dType)
    Q_UNUSED(sharedObjs)

    // mainTxt() is convenience function that sends a TEXT frame to the CmdExecutor
    // object to be processed by the host. it's basically a short hand for the emit
    // call:

    // emit dataToClient(toTEXT("some text\n"), TEXT);

    // errTxt() does the same thing but indicates to the client that it is a error
    // message so it can be displayed in a different color, size, logged etc..
    // depending on the client.

    // emit dataToClient(toTEXT("some error\n"), ERR);

    // privTxt() this also does that same thing except it indicates to the client
    // that the command is asking for private data like a password, SSN, pin number
    // etc.. clients that get this indicator should not echo or display the next
    // data frame to be sent back to the host.

    // emit dataToClient(toTEXT("enter your password: "), PRIV);

    mainTxt("Main text out from module: " + QString(LIB_NAME) + "\n");
    errTxt("Error text out from module: " + QString(LIB_NAME) + "\n");
}

void ModInput::procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType)
{
    Q_UNUSED(sharedObjs)

    if (dType == TEXT)
    {
        QString text = QTextCodec::codecForName(TXT_CODEC)->toUnicode(data);

        if (inMoreInputMode)
        {
            if (text.isEmpty())
            {
                errTxt("err: You entered nothing.\n");
            }
            else
            {
                if ((text.toLower() == "yes") || (text.toLower() == "no"))
                {
                    // setting moreInput false tells the host that the command
                    // is no longer asking for more input from the client so the
                    // host will consider the command finished at this point.

                    emit enableMoreInput(false);

                    mainTxt("You entered: '" + text + "' Good bye.\n");
                }
                else
                {
                    errTxt("err: Invalid response.\n");
                }
            }
        }
        else
        {
            // setting moreInput true tells the host that the command is not
            // finished and is awaiting more input from the client. you don't
            // need to reimplement term() if all it takes for the command to
            // finish its task is to set moreInput and/or loop false; the
            // host will do that externally.

            emit enableMoreInput(true);

            mainTxt("Please enter Yes or No: ");
        }
    }
}

void ModLoop::term()
{
    // this function is called by the host to terminate the command if
    // termination is requested and only if the more input or loop modes
    // are active.

    // this command has a variable called 'index' that the host is not aware
    // of and does not have access to it so in this case, index needs reset
    // to 0 when the command is requested to terminate.

    index = 0;
}

void ModLoop::procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType)
{
    Q_UNUSED(data)
    Q_UNUSED(dType)
    Q_UNUSED(sharedObjs)

    // the host will constantly call this function for as long as loop
    // mode remains true or if requested to terminate. keep in mind that
    // in all subsequent calls after the initial call, the input data will
    // always be empty and dType will always default to TEXT.

    if (inLoopMode)
    {
        mainTxt("Loop: " + QString::number(index++) + "\n");

        if (index == 10)
        {
            emit enableLoop(false);

            index = 0;
        }
    }
    else
    {
        emit enableLoop(true);
    }
}

QStringList ModInherit::internRequest()
{
    return QStringList() << "my_info";
}

void ModInherit::procBin(const SharedObjs *sharedObjs, const QByteArray &data, uchar dType)
{
    Q_UNUSED(data)
    Q_UNUSED(dType)

    if (!internCommands.contains("my_info"))
    {
        errTxt("err: This command object did not successfully inherit 'my_info' unable to continue.\n");
    }
    else
    {
        mainTxt("If inherited correctly, the output of 'my_info' should show below:\n\n");

        internCommands["my_info"]->procBin(sharedObjs, QByteArray(), TEXT);
    }
}
