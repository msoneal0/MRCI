### 6.1 Session/Host Shared Memory ###

The host use shared shared memory segments to share data with the module processes so the modules can be made aware of various session data points like the currently logged in user id, user name, display name, etc. A shared memory segment is basically a block of memory where multiple processes have access to reading/writing data. The use of shared memory segments is entirely optional so it does not need to be implemented to have a fully functional module.

The session object creates the shared memory segment and assigns a text key unique to just the session. When the session object calls a new module process, it will pass the [-mem_ses](modules.md) option with the shared memory key that the module can then use to attach to the segment. The session will also pass the [-mem_host](modules.md) option unique to the TCPServer object to get data relevant to the sever object.

As mentioned before, the share memory segments are just blocks of memory. sections 6.2 and 6.3 describes the format of the shared memory and where to find the data based on the memory offset.

Also note, depending on the API used, lock and unlock functions must be used when reading/writing data with the shared memory segments as a way to prevent multiple process from reading/writing the shared data at the same time. Consider reading your API's documentation before attempting to use shared memory segments.

### 6.2 Session Shared Memory Offsets ###

| Offset | Data                   | Type            | Bytes | Descrition                                                                   |
| ------ | ---------------------- | --------------- | ----- | ---------------------------------------------------------------------------- |
| 0      | Session ID             | Hash            | 28    | a unique hash for the current session.                                       |
| 28     | User ID                | Hash            | 32    | a unique hash for the currently logged in user account.                      |
| 60     | Client IP              | String          | 78    | the ip address of user's client in string form.                              |
| 138    | Client App             | String          | 134   | the name of the application the user is currently using.                     |
| 272    | User Name              | String          | 48    | current user name.                                                           |
| 320    | Disp Name              | String          | 64    | current display name.                                                        |
| 384    | Host Rank              | uint32          | 4     | current host rank.                                                           |
| 388    | Active Update          | uint8           | 1     | a bool value 0x00 or 0x01 if the session has an active update sub-ch open.   |
| 389    | Owner Override         | uint8           | 1     | also a bool value if the session has the channel owner override flag active. |
| 390    | Channel List           | uint64 (x200)   | 1600  | a list of up to 200 channel ids that the current user is a member of.        |
| 1990   | Open Sub-channels      | [5.3](async.md) | 54    | a list of sub-channels the session currently have open.                      |
| 2044   | Writeable Sub-channels | [5.3](async.md) | 54    | same as the list above except the sub-channels do not have read-only flags.  |
| 2098   | Pending P2P Request    | Hash (x100)     | 2800  | a list of up to 100 session ids that have pending p2p request.               |
| 4898   | Accepted P2P Request   | Hash (x100)     | 2800  | same as above except this list p2p request that was accepted.                |

notes:

* The "String" data type is [TEXT](type_ids.md) padded with 0x00.
* The list data types are also padded to strings of 0x00 based on the size of each sub-unit.

### 6.3 Host Shared Memory Offsets ###

| Offset | Data                   | Type            | Bytes | Descrition                                                                   |
| ------ | ---------------------- | --------------- | ----- | ---------------------------------------------------------------------------- |
| 0      | Number Of Sessions     | uint32          | 4     | the number of active concurrent sessions.                                    |

### 6.4 Data Constraints Table ###

| Data                   | Type   | Vaild Size Range | Forbidden Chars    | Must Contain                                                |
| ---------------------- | ------ | ---------------- | ------------------ | ----------------------------------------------------------- |
| Module Executable Path | String | 1-512            | ```|*:\"?<>```     |                                                             |
| User Name              | String | 2-24             | spaces or newlines |                                                             |
| Email Address          | String | 4-64             | spaces or newlines | ```@``` and ```.```                                         |
| Password               | String | 8-200            |                    | special chars, numbers, capital letters and common letters. |
| Command Name           | String | 1-64             | spaces or newlines |                                                             |
| Common Name            | String | 1-136            | spaces or newlines |                                                             |
| Display Name           | String | 0-32             | newlines           |                                                             |
| Channel Name           | String | 4-32             | spaces or newlines |                                                             |
| Sub-channel Name       | String | 4-32             | spaces or newlines |                                                             |
| Client App Name        | String | 0-67             |                    |                                                             |
| Command Lib Name       | String | 0-64             |                    |                                                             |
| IP Address (string)    | String | 3-39             | spaces or newlines | numbers and ```.``` or numbers, A-F and ```:```             |
| User ID                | Hash   | 32-32            |                    |                                                             |
| Session ID             | Hash   | 28-28            |                    |                                                             |
| Host Rank              | int32  | 1-4x10^9         | non-numeric chars  |                                                             |
| Max Sessions           | int32  | 1-4x10^9         | non-numeric chars  |                                                             |
| Channel ID             | int64  | 1-18x10^18       | non-numeric chars  |                                                             |
| Command ID             | int16  | 256-65535        | non-numeric chars  |                                                             |
| Async Command ID       | int16  | 1-255            | non-numeric chars  |                                                             |
| Sub-channel ID         | int8   | 0-255            | non-numeric chars  |                                                             |
| Channel Level          | int8   | 1-5              | non-numeric chars  |                                                             |

