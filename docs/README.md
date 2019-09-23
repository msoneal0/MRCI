# MRCI #

(Modular Remote Command Interpreter) is a command interpreter primarily designed to provide remote service to connected clients, whether text based or any kind of data. As the name implies, it is expandable via 3rd party modules by adding addtional commands that remote clients can run on the host. It has a fully feasured user account management system with access control to certain commands for certain user groups. All persistent data is handled by a SQLite database and all remote connections are handled via TCP and encrypted in SSL/TLS.

### Usage ###

```
Usage: mrci <arguments>

<Arguments>
  
  -help                   : display usage information about this application.
  -start                  : start a new host instance in the background.
  -stop                   : stop the current host instance if one is currently running.
  -about                  : display versioning/warranty information about this application.
  -addr {ip_address:port} : set the listening address and port for TCP clients.
  -status                 : display status information about the host instance if it is currently running.
  -reset_root             : reset the root account password to default.
  -executor               : this starts a command executor instance. this is normally for internal use only.
  -host                   : this starts a blocking host instance. this is normally for internal use only.
```
 
Other than that, the host can only be managed via a connected client that supports text input/output so the host application is always listening for clients while running entirely in the background. By default, the host listen for clients on address 127.0.0.1 and port 35516. Just like any linux OS, the host will have an administrative user called root along with a group also called root. This account can be used to modify anything on the host without restriction. The default password set to (R00tPa$$w0rd) when logging in for the first time as root, the host will require you to change the password before continuing.

### More Than Just a Command Interpreter ###

Typical use for a MRCI host is to run commands that clients ask it to run, very similar to what you see in terminal emulators. It however does have a few feasures typically not seen in local terminals:

* Broadcast any type of data to all peers connected to the host.
* Run remote commands on connected peers.
* Host object positioning data for peers (online games do this).
* Send data to/from a peer client directly.
* Fully feasured builtin user account management.
* Built in permissions and command access management.
* Host limits management (max concurrent users, max failed password attempts, etc...).
* Account recovery emailing in case of forgotten passwords. **
* Acesss to various logs.
* Built in host file management (copy, move, delete, etc...).
* Built in file upload/download support.

Because the host is modular, the things you can customize it to do is almost limitless by simply adding more commands.

** The email system of this application depends on external email clients that run on the command line. The default is [mutt](http://www.mutt.org/). If you want emails to work out of the box, consider installing and configuring mutt. It just needs to be configured with a smtp account to send emails with. You don't have to use mutt though, the host does have options to change the email client to any other application that has a command line interface.

### Documentation ###

* [1.1 The Protocol](Protocol.md)
* [2.1 Command Loaders](Command_Loaders.md)
* [3.1 Command Objects](Command_Objects.md)
* [4.1 Type IDs](Type_IDs.md)
* [5.1 Host Features](Host_Features.md)
* [6.1 Async Commands](Async.md)
* [7.1 Internal Commands](Internal_Commands.md)

### Development Setup ###

Linux Required Packages:
```
qtbase5-dev
libssl-dev
gcc
make
makeself
```

### Build From Source (Linux) ###

Linux_build.sh is a custom script designed to build this project from the source code using qmake, make and makeself. You can pass 2 optional arguments:

1. The path to the QT bin folder in case you want to compile with a QT install not defined in PATH.
2. Path of the output makeself file (usually has a .run extension). If not given, the outfile will be named mrci-1.1.2.run in the source code folder.

Build:
```
cd /path/to/source/code
sh ./linux_build.sh
```
Install:
```
chmod +x ./mrci-1.1.2.run
./mrci-1.1.2.run
```

The makeself installer not only installs the application but also installs it as a service if the target linux system supports systemd.

Start/Stop the service:
```
sudo systemctl start mrci@$USER
sudo systemctl stop mrci@$USER
```
