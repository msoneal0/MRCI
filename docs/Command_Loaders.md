### 2.1 Command Loaders ###

Every command object ([Command_Objects](Command_Objects.md)) defined in the host must have a unique command name atteched to it. Command loaders have the very simple job of creating these objects via the requested command name. The host use command names to determine if the current session is allowed to load them or not and command loaders are used to relate those command names with the command objects themselves.

When the host ```CmdExecutor``` determines that it can indeed load the commands based on the command names, it will then assign command ids to all command objects that were successfully built from the command loader. Each command loader can have a total of 256 command objects. Internally defined commands start at command id: 256 - 512 and module defined commands start at command id: 513 and up.

### 2.2 CommandLoader Class ###

The ```CommandLoader``` class itself defines a few virtual functions that make the command name to command object relationship possible:

```ExternCommand *cmdObj(QString cmdName)```
This function is used to create the ```ExternCommand``` object associated with the command name passed by the ```QString``` parameter and return a pointer to it. If the command name is invalid, doesn't exists or fails to contruct the object for some reason, return ```nullptr```. The command object can have a parent or orphaned, just make sure it never gets deleted at any time; the host will handle that externally. It is safe to use the command loader itself as the parent.

```QStringList cmdList()```
This function needs to return a ```QStringList``` of all of command names that this loader can actually load when the ```cmdObj()``` function is called. the host uses this list to enforce user access filtering of the command objects using the built in permission id system.

```QStringList pubCmdList()```
This funtion needs to work the same way as ```cmdList()``` except the loader can this function to name any commands that can be accessed by un-logged in clients. aka, public commands. Note: the commands listed here must also be listed in ```cmdList()``` or else the commands will not get loaded at all.

```QStringList rankExemptList()```
The loader can use this function to return a ```QStringList``` of all of command names that need to be exempt from host ranking system (section [5.2](Host_Features.md)). Commands listed here will be allowed to load/run regardless of what host rank the current user is.

```bool hostRevOk(quint64 rev)```
When the host calls this function, it will pass the import rev that it supports in the ```quint64``` parameter. Use this function to return if this rev is acceptable or not. The host will give up loading the module if the rev is not acceptable.

```quint64 rev()```
Use this function to return the import rev that this module supports. The host will decide if it is acceptable or not.

```QString lastError()```
The host will call this function if a command object fails to load or if ```hostRevOk()``` returns false so it can log the error message returned by it to the host database.

```void modPath(QString path)```
The host will call this function after successfully negotiating the import rev. The ```QString``` parameter passed into this will have the absolute path to the module's install directory. You can use this path to load additional files that came bundled the module.

```void aboutToDelete()```
The host will call this function before calling ```deleteLater()```. All command objects at this point should already be deleted, use this opportunity to free any resources related to the loader itself. Unload any additional lib files that the loader may have used.

Here's a few notes to consider when using this class:

* Never self delete or explicitly delete the command loader or any of the command objects it sucessfully creates at any time. The host ```CmdExecutor``` object will handle the life cycle of these objects externally.
* Command ids are assigned in alphabetical order of the command names returned by ```cmdList()```. The command ids will remain constant as long as the command names also remain constant. All clients are recommended to rely on the ASYNC_ADD_CMD and ASYNC_RM_CMD async commands to keep track of the command ids and names (section [6.2](Async.md)).

### 2.3 Modules ###

External commands are added to the host through modules based on low level [QT plugins](https://doc.qt.io/qt-5/plugins-howto.html). Each module must define a ```CommandLoader``` class in it's main library file and the file itself must be named 'main' with a library file extension that the host platform supports (main.so, main.dll, etc..). Modules are installed using the *add_mod* internal command that supports extracting the module's library files from an archive file (.zip, .tar, etc...) or just a single library file (.so, .dll, .a, etc...).

In the case of an archive file, it extracts all of the files from the the archive file while preserving the directory tree so you can bundle additional files that your module depends on but as mentioned before, a library file named 'main' must be present on the base directory.

A template and an example of a module can be found in the 'modules/Tester' directory of the source code of this project. It provides the command.cpp and command.h files that contain the ```CommandLoader``` and ```ExternCommand``` classes that are needed to create a module. Also feel free to copy the command.cpp and command.h files from 'src/commands' if you prefer.

### 2.4 The Import Rev ###

The import rev is a single digit versioning system for external modules that help the host determine if it is compatible with the module it is attempting to load or not. Bumps to this rev is usually triggered by significant changes to the ```CommandLoader``` class. Compatibility negotiation is a two way communication between the host ```CmdExecutor``` and the module itself using the virtual functions described in section 2.2.