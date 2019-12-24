# MRCI #

(Modular Remote Command Interpreter) is a command interpreter primarily designed to provide any type of remote service to connected clients. As the name implies, it is expandable via 3rd party modules by adding addtional commands that remote clients can run on the host. It has a fully feasured user account management system with access control to certain commands for certain users. All persistent data is handled by a SQLite database and all remote connections are handled via TCP and encrypted in SSL/TLS.

### Usage ###

```
Usage: mrci <argument>

<Arguments>

 -help                   : display usage information about this application.
 -stop                   : stop the current host instance if one is currently running.
 -about                  : display versioning/warranty information about this application.
 -addr {ip_address:port} : set the listening address and port for TCP clients.
 -status                 : display status information about the host instance if it is currently running.
 -reset_root             : reset the root account password to the default password.
 -host                   : start a new host instance. (this blocks).
 -public_cmds            : run the internal module to list it's public commands. for internal use only.
 -exempt_cmds            : run the internal module to list it's rank exempt commands. for internal use only.
 -user_cmds              : run the internal module to list it's user commands. for internal use only.
 -run_cmd {command_name} : run an internal module command. for internal use only.

Internal module | -public_cmds, -user_cmds, -exempt_cmds, -run_cmd |:

 -pipe {pipe_name/path} : the named pipe used to establish a data connection with the session.
 -mem_ses {key_name}    : the shared memory key for the session object.
 -mem_host {key_name}   : the shared memory key for the server object.
```
 
The host can only be managed via a connected client that supports text input/output so the host application is always listening for clients while running entirely in the background. By default the host listen for clients on address 0.0.0.0 and port 35516, effectively making it reachable on any network interface of the host platform via that specific port.

Just like any linux based OS, the host will have an administrative user called root. This account can be used to modify anything on the host without restriction. The default password is randomized and set on the root user account upon running the host for the first time. To find out what the default password is, run -help or -about. When logging in as root with the default password, the host will require you to change the password before continuing.

### More Than Just a Command Interpreter ###

Typical use for a MRCI host is to run commands that clients ask it to run, very similar to what you see in terminal emulators. It however does have a few feasures typically not seen in local terminals:

* Broadcast any type of data to all peers connected to the host.
* Run remote commands on connected peers.
* Host object positioning data for peers (online games do this).
* Send data to/from a peer client directly.
* Fully feasured user account management system.
* Built in permissions and command access management.
* Host limits management (max concurrent users, max failed password attempts, etc...).
* Account recovery emailing in case of forgotten passwords. **
* Acesss to various logs.
* Built in host file management (copy, move, delete, etc...).
* Built in file upload/download support.

Because the host is modular, the things you can customize it to do is almost limitless by simply adding more commands.

** The email system of this application depends on external email clients that run on the command line. The default is [mutt](http://www.mutt.org/). If you want emails to work out of the box, consider installing and configuring mutt. It just needs to be configured with a smtp account to send emails with. You don't have to use mutt though, the host does have options to change the email client to any other application that has a command line interface.

### Documentation ###

* [1.1 The Protocol](protocol.md)
* [2.1 Modules](modules.md)
* [3.1 Type IDs](type_ids.md)
* [4.1 Host Features](host_features.md)
* [5.1 Async Commands](async.md)
* [6.1 Shared Memory](shared_data.md)
* [7.1 Internal Commands](intern_commands.md)

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
2. Path of the output makeself file (usually has a .run extension). If not given, the outfile will be named mrci-x.x.x.run in the source code folder.

Build:
```
cd /path/to/source/code
sh ./linux_build.sh
```
Install:
```
chmod +x ./mrci-x.x.x.run
./mrci-x.x.x.run
```

The makeself installer not only installs the application but also installs it as a service if the target linux system supports systemd.

Start/Stop the service:
```
sudo systemctl start mrci@$USER
sudo systemctl stop mrci@$USER
```
