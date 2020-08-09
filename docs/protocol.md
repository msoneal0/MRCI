### 1.1 The Protocol ###

The main goal of this application is to transport data from remote TCP clients to the [Modules](modules.md) defined in the host. How the data is processed and/or returned back to the client depends entirely on the type of data being transported or the module itself. The data format that the host understands for data transport is referred to as MRCI frames described below in section 1.2.

Before any MRCI frames can be transported, both the host and client need basic information about each other. This is done by having the client send a fixed length client header when it successfully connects to the host and the host will reply with it's own fixed length host header. Descriptions of these headers can be found in sections 1.4 and 1.5. 

### 1.2 MRCI Frames ###

```
[type_id][cmd_id][branch_id][data_len][payload]

type_id   - 1byte    - 8bit little endian integer type id of the payload.
cmd_id    - 2bytes   - 16bit little endian integer command id.
branch_id - 2bytes   - 16bit little endian integer branch id.
data_len  - 3bytes   - 24bit little endian integer size of the payload.
payload   - variable - the actual data to be processed.
```

notes:

* A full description of the type id's can be found in the [Type_IDs.md](type_ids.md) document.

* Modules call commands via a command name but the host will assign a unique command id to all command names so clients can call them using a simple 2 byte integer instead of full text. The command ids can change as the modules change so it is recommended for clients to not depend on consistant command ids but depend on the [ASYNC_ADD_CMD](async.md) and [ASYNC_RM_CMD](async.md) async commands.

* The branch id is an id that can be assigned by the client itself to run muliple instances of the same command. Commands sent by a certain branch id will result in data sent back to the client from the module with that same branch id.

### 1.3 Versioning System ###

The host uses a 4 number versioning system that indicate rev numbers for the host application itself, the tcp interface and the module interface:
```
[Major][Minor][TCP_Rev][Mod_Rev]
   3  .   2  .   1    .    0
   
Major - this indicate any changes to the host application that would cause 
        clients to need to change behaviour to maintain compatibility.
        changes to the core command names, type id format changes, etc.
        will cause the version major to increment.
        
Minor - this indicate any changes to the host application that clients will
        not see and would not need behaviour changes to maintain
        compatibility. documentation changes, bug fixes, security patches,
        etc. will cause the version minor to increment.
        
TCP_Rev - this indicate any changes to the MRCI protocol that interface the
          host with the clients via the TCP connection. any changes to the
          MRCI frames, host/client headers, etc. will cause this rev to
          increment.
          
Mod_Rev - this indicate any changes to the IPC protocol that interface the
          host with the modules via named pipes. any changes to the IPC 
          frames, NEW_CMD/CMD_ID type ids, etc. will cause this rev to
          increment.
   
```

Any increments to the Major resets the Minor to 0. Any 3rd party client applications connecting to a MRCI host need to be aware of this versioning but does not need to adopt it as it's own version number.

### 1.4 Client Header ###

```
[tag][appName][mod_instructions][padding]

tag     - 4bytes   - 0x4D, 0x52, 0x43, 0x49 (MRCI)
appName - 32bytes  - UTF8 string (padded with 0x00)
modInst - 128bytes - UTF8 string (padded with 0x00)
padding - 128bytes - string of (0x00)
```

notes:

* **tag** is just a fixed ascii string "MRCI" that indicates to the host that the client is indeed attempting to use the MRCI protocol.

* **appName** is the name of the client application that is connected to the host. It can also contain the client's app version if needed because it doesn't follow any particular standard. This string is accessable to all modules so the commands themselves can be made aware of what app the user is currently using.

* **modInst** is an additional set of command lines that can be passed onto to all module processes when they are intialized. This can be used by certain clients that want to intruct certain modules that might be installed in the host to do certain actions during intialization. This remains constant for as long as the session is active and cannot be changed at any point.

### 1.5 Host Header ###

```
Format:

[reply][major][minor][tcp_rev][mod_rev][sesId]

reply   - 1byte   - 8bit little endian unsigned int
major   - 2bytes  - 16bit little endian unsigned int
minor   - 2bytes  - 16bit little endian unsigned int
tcp_rev - 2bytes  - 16bit little endian unsigned int
mod_rev - 2bytes  - 16bit little endian unsigned int
sesId   - 28bytes - 224bit sha3 hash
```

notes:

* **reply** is a numeric value that the host returns in it's header to communicate to the client if SSL need to initated or not.

    * reply = 1, means SSL is not required so the client doesn't need to take any further action.
    * reply = 2, means SSL is required to continue so the client needs to send a STARTLS signal.

* **sesId** is the session id. It is a unique 224bit sha3 hash generated against the current date and time of session creation (down to the msec) and the machine id of the host. This can be used by the host or client to uniquely identify the current session or past sessions.