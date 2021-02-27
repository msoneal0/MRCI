# MRCI #

(Modular Remote Command Interpreter) is a command interpreter primarily designed to provide any type of remote service to connected clients. As the name implies, it is expandable via 3rd party modules by adding addtional commands that remote clients can run on the host. It has a fully feasured user account management system with access control to certain commands for certain users.

### Usage ###

```
Usage: mrci <argument>

<Arguments>

 -help        : display usage information about this application.
 -stop        : stop the current host instance if one is currently running.
 -about       : display versioning/warranty information about this application.
 -status      : display status information about the host instance if it is currently running.
 -host        : start a new host instance. (this blocks)
 -host_trig   : start a new host instance. (this does not block)
 -public_cmds : run the internal module to list it's public commands. for internal use only.
 -exempt_cmds : run the internal module to list it's rank exempt commands. for internal use only.
 -user_cmds   : run the internal module to list it's user commands. for internal use only.
 -run_cmd     : run an internal module command. for internal use only.
 -ls_sql_drvs : list all available SQL drivers that the host currently supports.
 -load_ssl    : re-load the host SSL certificate without stopping the host instance.
 -elevate     : elevate any user account to rank 1.
 -add_admin   : create a rank 1 account with a randomized password.

Internal module | -public_cmds, -user_cmds, -exempt_cmds, -run_cmd |:

 -pipe     : the named pipe used to establish a data connection with the session.
 -mem_ses  : the shared memory key for the session.
 -mem_host : the shared memory key for the host main process.

Details:

add_admin - this argument takes a single string representing a user name to create a rank 1 account with.
            the host will set a randomized password for it and display it on the CLI. this user will be
            required to change the password upon logging in.
            example: -add_admin somebody

elevate   - this argument takes a single string representing a user name to an account to promote to rank 1.
            example: -elevate somebody

run_cmd  - this argument is used by the host itself, along side the internal module arguments below to run
           the internal command names passed by it. this is not ment to be run directly by human input.
           the executable will auto close if it fails to connect to the pipe and/or shared memory segments
```
 
The host can be managed via a connected client that supports text input/output so the host application is always listening for clients while running entirely in the background. By default the host listen for clients on address 0.0.0.0 and port 35516, effectively making it reachable on any network interface of the host platform via that specific port.

### More Than Just a Command Interpreter ###

Typical use for a MRCI host is to run commands on a remote host that clients ask it to run, very similar to what you see in remote terminal emulators. It however does have a few feasures typically not seen in terminals:

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

** The email system of this application depends on external email clients that run on the command line. The default is [mutt](http://www.mutt.org/). If you want emails to work out of the box, consider installing and configuring mutt. It just needs to be configured with a smtp account to send emails with. You don't have to use mutt though, the host does have the option to change the email client to any other application that has a command line interface.

### Documentation ###

* [1.1 The Protocol](protocol.md)
* [2.1 Modules](modules.md)
* [3.1 Type IDs](type_ids.md)
* [4.1 Host Features](host_features.md)
* [5.1 Async Commands](async.md)
* [6.1 Shared Memory](shared_data.md)
* [7.1 Internal Commands](intern_commands.md)

### Build Setup ###

For Linux you need the following packages to successfully build/install:
```
qtbase5-dev
libssl-dev
gcc
make
python3
```

For Windows support you need to have the following applications installed:
```
OpenSSL
Qt5.12 or newer
Python3
```

### Build ###

To build this project from source you just need to run the build.py and then the install.py python scripts. While running the build the script, it will try to find the Qt API installed in your machine according to the PATH env variable. If not found, it will ask you to input where it can find the Qt bin folder where the qmake executable exists or you can bypass all of this by passing the -qt_dir option on it's command line.

while running the install script, it will ask you to input 1 of 3 options:

***local machine*** - This option will install the built application onto the local machine without creating an installer.

***create installer*** - This option creates an installer that can be distributed to other machines for installation. The resulting installer is just a regular .py script file that the target machine can run if it has Python3 insalled. Only Python3 needs to be installed and an internet connection is not required.

***exit*** - Cancel the installation.

-local or -installer can be passed as command line options for install.py to explicitly select one of the above options without pausing for user input.

### Services ###

If a target linux system supports systemd, the application will be installed as a background daemon that can start/stop with the following commands:
```
sudo systemctl start mrci
sudo systemctl stop mrci
```

In a Windows system, a scheduled task will be created to auto start the application in the background when the system starts. 
