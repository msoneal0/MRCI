### 3.1 Type IDs ###

All mrci frames transferred throughout this application have an 8bit numeric value to indicate the type of data being passed with the binary data. The type id enum values are as follows:

```
enum TypeID : quint8
{
    GEN_FILE       = 1,
    TEXT           = 2,
    ERR            = 3,
    PRIV_TEXT      = 4,
    IDLE           = 5,
    FILE_INFO      = 7,
    PEER_INFO      = 8,
    MY_INFO        = 9,
    PEER_STAT      = 10,
    P2P_REQUEST    = 11,
    P2P_CLOSE      = 12,
    P2P_OPEN       = 13,
    BYTES          = 14,
    SESSION_ID     = 15,
    NEW_CMD        = 16,
    CMD_ID         = 17,
    BIG_TEXT       = 18,
    TERM_CMD       = 19,
    HOST_VER       = 20,
    PING_PEERS     = 21,
    CH_MEMBER_INFO = 22,
    CH_ID          = 23,
    KILL_CMD       = 24,
    YIELD_CMD      = 25,
    RESUME_CMD     = 26,
    PROMPT_TEXT    = 27,
    PROG           = 28,
    PROG_LAST      = 29,
    ASYNC_PAYLOAD  = 30
};
```

### 3.2 Type Descriptions ###
    
```TEXT```
This is text that can be displayed directly to the user, passed as command line arguments to be processed or used to carry text data within other data types.

format: ```[UTF-16LE_string] (no BOM)```

```GEN_FILE```
This is a file transfer type id that can be used to transfer any file type (music, photos, documents, etc...). It operates in its own protocol of sorts. The 1st GEN_FILE frame received by the host or client is TEXT parameters similar to what you see in terminal command lines with at least one of the arguments listed below. The next set of GEN_FILE frames received by the host or client is then the binary data that needs to be written to an open file or streamed until the limit defined in -len is meet.

The host or the client can be set as the sender or receiver of the GEN_FILE binary data. This designation is determined by what the command defined in genfile type when the NEW_CMD frame was sent. A genfile type of 2 sets the client as the sender and the host as the receiver. Genfile type 3 sets the client as the receiver and the host as the sender.

see section 3.3 for an example of how GEN_FILE works.

arguments:

* **-len (int)** | this is the integer value of the file size or amount of bytes to read/write.

* **-offset (int)** | this is a integer position that indicates where in the source or destination file to start reading/writing.

* **-client_file** (string) | this is the file path to the source/destination file in the client's file system.

* **-remote_file** (string) | this is the file path to the source/destination file in the host file system.

* **-single_step** | the presents of this argument tells both the client and host to operate in single step mode. single step mode causes the receiver of the binary data whether host or client to send an empty GEN_FILE frame after successfully receiving the data. this then tells the sender to send the next GEN_FILE frame containing binary data for the file and the cycle continues until len is meet. if this argument is not found, the sender can simply send all GEN_FILE data without waiting for an empty GEN_FILE from the receiver.

* **-truncate** | this indicates to whoever is the receiver to truncate the file being written to.

* **-force** | in some cases, the receiver might need to overwrite the target file. the presents of this argument tells it to overwrite without asking the user. the host should never send this argument and the client should ignore it if it is received from the host.

```ERR```
This type id is similar to TEXT except it indicates that this is an error message that can be displayed directly to the user if needed.

```PRIV_TEXT```
This id can be treated exactly like TEXT except this tells the client that the command is prompting for private information from the user so it sould hide or do not echo the next TEXT data that the command is expecting, like a password or other sensitive data.

```PROMPT_TEXT```
This is similar to PRIV_TEXT expect it is not asking for private information. It is simply prompting for non-sensitive information from the user.

```BIG_TEXT```
Also formatted exactly like TEXT but this indicates to the client that this is a large body of text that is recommended to be word wrapped when displaying to the user. It can contain line breaks so clients are also recommended to honor those line breaks.

```IDLE```
All commands started during the session returns this type id when it has finished it's task. It carries a 16bit unsigned integer indicating the result of the task that the command was running.

```
enum RetCode : quint16
{
    NO_ERRORS       = 1, // task execution completed without any issues.
    ABORTED         = 2, // the task aborted via user or host intervention.
    INVALID_PARAMS  = 3, // invalid/missing parameters prevented the task from executing.
    CRASH           = 4, // the command process has crashed.
    FAILED_TO_START = 5, // the command process could not start.
    EXECUTION_FAIL  = 6, // command specific error prevented execution of the task.
    CUSTOM          = 7  // indicates a custom return code.
};

notes:
1. the custom return code can be additional data added to the end of the 16bit
   integer that can carry additional data about the result of the task. it can
   be any format that the module itself decides it can should be. nothing is
   stopping modules from defining return codes beyond the value of 7 but it is
   advised not to because this enum might be expanded in the future.
``` 

```TERM_CMD```
This type id doesn't carry any actual data. It is used to tell the host to stop/terminate the command id and branch id that was used to send it. It does not actually terminate the module's process within the host, it only simply tells it to stop what it is currently doing. This will also terminate any commands in a prompt/more input state.

```KILL_CMD```
This works similarly to TERM_CMD except it will also terminate the module process. The module process will have 3 seconds to shutdown gracefully before it is force killed by the host session.

```YIELD_CMD```
This type id doesn't carry any actual data, instead can be used to tell the host to pause/yield the command id and branch id that was used to send it. All modules are not obligated to support this feature but it is highly recommended to do so.

```RESUME_CMD```
This is the other half of YIELD_CMD that tells the host to resume the command that was running.

```HOST_VER```
This data structure carries 4 numeric values that represent the host version as described in section [1.3](protocol.md).

```
  format:
  1. bytes[0-1] - version major (16bit little endian uint)
  2. bytes[2-3] - version minor (16bit little endian uint)
  3. bytes[4-5] - tcp interface rev (16bit little endian uint)
  4. bytes[6-7] - module interface rev (16bit little endian uint)
```

```ASYNC_PAYLOAD```
This is a data structure that can be used by modules to run async commands. Each async command have a specific 16bit command id followed by a payload that can contain any data of any format depending on the async command id. A full discribtion of the async commands and the data they carry can found in section [5.1](async.md) and [5.2](async.md).

```
  format:
  1. bytes[0-1] - async command id (16bit little endian uint)
  2. bytes[2-n] - payload (data to be processed by async command)
```

```FILE_INFO```
This is a data structure that carries information about a file system object (file,dir,link).

```
  format:
  1. bytes[0]     - flags (8bit little endian uint)
  2. bytes[1-8]   - creation time in msec since Epoch UTC (64bit little endian uint)
  3. bytes[9-16]  - modification time in msec since Epoch UTC (64bit little endian uint)
  4. bytes[17-24] - file size (64bit little endian uint)
  5. bytes[25-n]  - file name (UTF16-LE string, 16bit terminated)
  6. bytes[n-n]   - symmlink target if it is a symmlink (UTF16-LE string, 16bit terminated)

  notes:
  1. 16bit terminated UTF-16LE strings are basically
     terminated by 2 bytes of 0x00.
     
  2. the symmlink target is empty if not a symmlink but
     the terminator should still be present.

  flags:
  1. bit 0 - true if the object is a file
  2. bit 1 - true if the object is a directory
  3. bit 2 - true if the object is a symmlink
  4. bit 3 - true if the current user have read permissions
  5. bit 4 - true if the current user have write permissions
  6. bit 5 - true if the current user have execute permissions
  7. bit 6 - true if the object exist in the file system. if symmlink,
             this determines if the symm target exists or not.
```

```PEER_INFO```
This carry some user account and session information about a peer client connected to the host.

```
  format:
  1. bytes[0-27]    28bytes  - session id (224bit hash)
  2. bytes[28-59]   32bytes  - user id (256bit hash)
  3. bytes[60-107]  48bytes  - user name (TEXT - padded with 0x00)
  4. bytes[108-241] 134bytes - app name (TEXT - padded with 0x00)
  5. bytes[242-305] 64bytes  - disp name (TEXT - padded with 0x00)

  notes:
  1. the session id is unique to the peer's session connection only. it
     can change upon reconnection.
     
  2. the user id is unique to the peer's user account. is stays constant
     even when the user name changes and across all clients logged into
     the same account.
     
  3. the display name is the preffered display name of the peer. clients
     are encouraged to use this rather than the user name when displaying
     peer info to the user. if empty, it's ok to just fall back to the user
     name.
```

```PING_PEERS```
This is formatted extactly as PEER_INFO except it is used by the ASYNC_LIMITED_CAST [async](async.md) command to tell all peer sessions that receive it to send a PEER_INFO frame about the local session to their own clients and return PEER_INFO frames about themselves to the local session.

```MY_INFO```
This contains all of the information found in ```PEER_INFO``` for the local session but also includes the following:

```
  format:
  1. bytes[306-433] 128bytes - email (TEXT - padded with 0x00)
  2. bytes[434-437] 4bytes   - host rank (32bit unsigned int)
  3. bytes[438]     1byte    - is email confirmed? (0x00 false, 0x01 true)
```

```NEW_CMD```
This contains information about a new command that was added to the current session.

```
  format:
  1. bytes[0-1]     2bytes   - 16bit LE unsigned int (command id)
  2. bytes[2]       1byte    - 8bit LE unsigned int (genfile type)
  3. bytes[3-130]   128bytes - command name (TEXT - padded with 0x00)
  4. bytes[131-258] 128bytes - library name (TEXT - padded with 0x00)
  5. bytes[259-n]   variable - short text (16bit null terminated)
  6. bytes[n-n]     variable - io text (16bit null terminated)
  7. bytes[n-n]     variable - long text (16bit null terminated)

  notes:
  1. the genfile type is numerical value of 2, 3 or 0. a value of 2 
     indicates that the command handles/understands the GEN_FILE mini 
     protocol and it can be used to upload a file or other data to the 
     host. a value of 3 indicates the commmand downloads a file or other
     data from the host. 0 simply indicates that the command doesn't use
     or understand GEN_FILE.
     
  2. the library name can contain the module name and/or extra informaion 
     the client can use to identify the library the command is a part of.
```

```CMD_ID```
This type id carries a 16bit unsigned LE int representing a command id.

format: ```2bytes - 16bit LE unsigned int (command id)```

```CH_ID```
This type id carries a 64bit unsighed LE int indicating the channel id.

format: ```8bytes - 64bit LE unsigned int (channel id)```

```SESSION_ID```
This is a fixed length 28byte(224bit) sha3 hash of a client's session id connected to the host. This is unique to just the client's tcp connection with the host. This changes upon re-connection.

format: ```28bytes - session id (224bit sha3 hash)```

```PEER_STAT```
This contain status information of a peer client when the peer changes sub-channels or disconnects from the host.

```
  format:
  1. bytes[0-27]  28bytes - session id (224bit hash)
  2. bytes[28-81] 54bytes - channel-sub ids
  3. bytes[82]    1byte   - is disconnected? (0x00 false, 0x01 true)

  notes:
  1. if (is disconnected) is set true (0x01) the session id will no longer
     be valid for that peer client so you should not make anymore attempts
     to send data to it.
     
  2. channel-sub ids is a string of 9byte channel-sub id combinations at
     a fixed length of 54bytes (padded with 0x00). this indicates what
     channels-subs the peer currently have open if the peer's channel ids
     no longer match with your session, it can be considered inactive or
     disconnected since you will no longer send/receive data with this peer.
```

```P2P_REQUEST```
This is formatted extactly like PEER_INFO except it is allowed to be sent directly to a peer session without retriction via the ASYNC_P2P [async](async.md) command. It will be up to the target peer to respond with a P2P_OPEN for the session to then unrestrict ASYNC_P2P so it will then be able to send/received other TypeIDs with this peer until P2P_CLOSE is sent/received. P2P_CLOSE can also be sent to decline the request.

```P2P_OPEN```
This contains a 28byte session id hash of the peer session that you or the peer will allow direct communication with ASYNC_P2P.

format: ```28bytes - session id (224bit sha3 hash)```

```P2P_CLOSE```
This is the other half of P2P_OPEN that will close direct communication with ASYNC_P2P.

format: ```28bytes - session id (224bit sha3 hash)```

```BYTES```
This contains arbitrary binary data of any format.

```CH_MEMBER_INFO```
This contains public information about a channel member.

```
  format:
  1. bytes[0-7]  8bytes   - channel id (64bit unsigned int)
  2. bytes[8-39] 32bytes  - user id (256bit hash)
  3. bytes[40]   1byte    - is invite? (0x00=false, 0x01=true)
  4. bytes[41]   1byte    - member's channel privilege level (8bit unsigned int)
  5. bytes[42-n] variable - user name (TEXT - 16bit null terminated)
  6. bytes[n-n]  variable - display name (TEXT - 16bit null terminated)
  7. bytes[n-n]  variable - channel name (TEXT - 16bit null terminated)
  
  notes:
  1. a 16bit null terminated TEXT formatted string ended with 2 bytes of
     (0x00) to indicate the end of the string data.
     
  2. the member's privilege level can be any of the values discribed in
     section [4.3](host_features.md).
     
  3. is invite? indicates if this user has received an invite to join
     that channel by has not accepted yet. if, accepted the user will
     become a full member of the channel at the level indicated by this
     data type.
```

```PROG```
This is a 8bit integer value from 0-100 indicating the percentage progress of the command. All long running module commands are encouraged to use this to update the client on command progress; sending it in a 1-5 second pulse rate.

format: ```1byte - 8bit integer (0-100)```

```PROG_LAST```
This is formatted and treated exactly like PROG except it indicates to the client that this is the last progress update for the current string of progress updates. So at the client side, progress strings/pulses should appear with a single or multiple PROG frame(s) and then end with PROG_LAST. Receiving an IDLE should also end the progress string if seen before PROG_LAST.

### 3.3 GEN_FILE Example ###

Setup:

* The host has a command called *upload_file* with a command id of *768* and handles the ```GEN_FILE``` data type with a genfile type of 2.
* The client has a file called */home/foo/bar.mp3* and wants to upload it to the host file */home/host/music/bar.mp3* and the client knows the file size is 512bytes.

Process:

To upload the file, the client calls command id *768* with the following text arguments (must still be sent as a GEN_FILE):
```-client_file "/home/foo/bar.mp3" -remote_file "/home/host/music/bar.mp3" -len 512```

The host can then return text arguments to the client like (also sent as a GEN_FILE):
```-truncate and/or -single_step```

This only needs to be done if the command call needs to be modified by host and if the client supports such a thing. In this example, host will just return an empty GEN_FILE because there is no need for modification.

At this point, the host will then need to check of the destination file: /home/host/music/bar.mp3 already exists. If it does, the host will need to ask the user if it's ok to overwrite.

Once the host confirms it is ok to write to the destination file, it will then need to send another empty GEN_FILE to the client to confirm that it is ready to start receiving GEN_FILE frames from the client through command id *768* that will contain binary data to be written to the destination file until -len (512 bytes) is meet.