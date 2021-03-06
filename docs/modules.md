### 2.1 Modules ###

Modules in this project are independent applications that communicate with the host application's session objects via named pipes and/or shared memory segments. The host will call multiple instances of these modules if needed using the command line options described in section 2.3 depending on session activity. The format for data transport between the session and module via the named pipe is a modified version of the MRCI frame described in section 2.2 called the IPC frame.

Basically the module's main job is to input and output IPC frames with the session object with the option to read session data from shared memory. The MRCI host have internal commands that are loaded using the module interface so the MRCI host project itself is a good example of a module. See the following classes and source files to understand how the internal module works and to understand how to make an external module based on it.

```
int main()      - main.cpp
class Module    - module.h, module.cpp
class MemShare  - mem_share.h, mem_share.cpp
class CmdObject - cmd_object.h, cmd_object.cpp
class IPCWorker - cmd_object.h, cmd_object.cpp
```

### 2.2 IPC Frame ###

```
[type_id][data_len][payload]

type_id  - 1byte    - 8bit little endian integer type id of the payload.
data_len - 3bytes   - 24bit little endian integer size of the payload.
payload  - variable - the actual data to be processed.
```

A full description of the type id's can be found in the [Type_IDs.md](type_ids.md) document.

### 2.3 Module Command Line Options ###

```
The host will call the module with just one of these parameters:

 -public_cmds            : send a NEW_CMD frame for all public commands the module can run.
 -exempt_cmds            : send a NEW_CMD frame all rank exempt commands the module can run.
 -user_cmds              : send a NEW_CMD frame for all user rank enforced commands the module can run.
 -run_cmd {command_name} : run a module command based on the command name.
 
The host will include all 3 of these parameters with the above option:

 -pipe {pipe_name/path} : the named pipe used to establish a data connection with the session.
 -mem_ses {key_name}    : the shared memory key for the session object.
 -mem_host {key_name}   : the shared memory key for the host main process.
```

notes:

* When the session calls the module in list mode (-public_cmds, -exempt_cmds or -user_cmds), it will only respond to frame type ids: [NEW_CMD](type_ids.md) or [ERR](type_ids.md) from the module; all other data types are ignored. Modules called in run mode (-run_cmd) on the other hand will open up all frame type ids.

* When the session detects that the module successfully established a pipe connection, it will send a [HOST_VER](type_ids.md) frame to the module so the module can decide if it supports the host. If the host is not compatible or the module fails for what ever other reason, the module can send a useful error message [ERR](type_ids.md) and then terminate gracefully. The error message will be added to the host debug log where it can be used by host admins to find out what went wrong. The HOST_VER frame is sent only when the module is called with the -public_cmds, -exempt_cmds or -user_cmds parameter.

* When the module sends a [NEW_CMD](type_ids.md) frame, the 16bit command id is needed but does not need to be valid, it just needs to be there as a place holder. The session will auto fill a valid command id before sending the data to the client. A valid NEW_CMD frame must have a minimum of 259 bytes and a valid command name. the session will ignore all NEW_CMD frames the doesn't meet these requirements. See section [6.3](shared_data.md) for what would be considered a valid command name.

* When a session starts, it will call all modules with the -public_cmds and it doesn't matter if the command names returned to the session overlap with -exempt_cmd or -user_cmds. When a user is logged in, it will then call 2 instances of each module with the -exempt_cmds and -user_cmds parameters so the command names should not overlap when these parameters are active.

* Modules called with -run_cmd does not need to terminate after running the requested command, instead it must send an [IDLE](type_ids.md) frame to indicate that the command is finished when it eventually does finish. This is desired because not only it tells the client that the command is finished but it also makes it so the session doesn't need to recreate the module process on every subsequent call to the command.

* The session will send a [KILL_CMD](type_ids.md) to the module after 2 mins of being idle (no IPC/Pipe activity). The module will have 3 seconds to do this before it is force killed. This will also happen when the user ends the session and the module process needs to terminate. Modules must still send an [IDLE](type_ids.md) frame to indicate the command is finished if a command was running.

### 2.4 Module Standard Output/Error ###

The host captures all text written to standard out/err. Although modules can send text data to clients via the [TEXT](type_ids.md) frame, another way to do the same thing is to write to stdout. The host however treats stderr differently, it sends all text written to stderr to the host logging system. On Linux systems, syslog is used. On windows systems, a local log file in %PROGRAMDATA%\mrci\messages.log is used. When logging messages, the host will send a generic error message to the client but the full error details will be sent to the logs along with the a generated message id and the module executable.