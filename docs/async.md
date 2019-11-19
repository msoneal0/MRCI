### 5.1 Async Commands ###

An async command is a virtual command that the host can use to send data to the client at any time while connected to the host. As the name implies, the occurance of a client receiving data from an async command is not always the result of running a regular command in the current session. This can occur for example when information in your account is changed by another client connected to the host; your client would not know about this change until an async command is sent notify it of the change. These commands can be called directly or indirectly by a module and are considered "virtual" commands because there is no defined objects attached to them. Instead, async commands are best identified by command id values 1-255.

Async commands are not only used to send data to the client but also used internally within the host to help objects operating in different processes to communicate with each other. Some async commands in fact are considered internal only because the client should never see any data come from them at anytime. There is also data flow contriants for aysnc commands, meaning some gets blocked if sent from the module or has no effect if sent with the unexpected [IPC](type_ids.md) type id. The list below shows the various data flow contriants each of these async commands have.

Here is a describtion of what the keywords in the list mean:
```
client    - this means the async command id will be used to forward 
            data of any type to client if needed.
           
internal  - this means the async command will be responded by the 
            session object but the data will not be forwarded to the 
            client or converted to an entirely different async 
            command before sending to the client.
           
public    - this means the session objects will respond to this async 
            command if sent with PUB_IPC or PUB_IPC_WITH_FEEDBACK
            from the module.
           
private   - this means only the session object that has a direct IPC
            connection with the module that sends the async command
            via PRIV_IPC will respond to it.
           
none      - this means none of the session objects will respond to
            this async command no matter which of the IPC data types
            are used. it is resevered for just the session object to 
            send to the client.
           
retricted - this means the session object will actively block this
            async command from being sent from the module (any mode).
```

```
enum AsyncCommands : quint16
{
    ASYNC_RDY               = 1,   // client   | none
    ASYNC_SYS_MSG           = 2,   // client   | none
    ASYNC_EXIT              = 3,   // internal | private
    ASYNC_CAST              = 4,   // client   | public
    ASYNC_MAXSES            = 5,   // internal | private
    ASYNC_LOGOUT            = 6,   // internal | private
    ASYNC_USER_DELETED      = 7,   // client   | public
    ASYNC_DISP_RENAMED      = 8,   // internal | public
    ASYNC_USER_RANK_CHANGED = 9,   // internal | public
    ASYNC_CMD_RANKS_CHANGED = 10,  // internal | public
    ASYNC_RESTART           = 11,  // internal | private
    ASYNC_ENABLE_MOD        = 12,  // internal | public
    ASYNC_DISABLE_MOD       = 13,  // internal | public
    ASYNC_END_SESSION       = 14,  // internal | private
    ASYNC_USER_LOGIN        = 15,  // internal | private
    ASYNC_TO_PEER           = 16,  // client   | public  | retricted
    ASYNC_LIMITED_CAST      = 17,  // client   | public
    ASYNC_RW_MY_INFO        = 18,  // internal | public
    ASYNC_P2P               = 19,  // client   | public
    ASYNC_CLOSE_P2P         = 20,  // internal | public
    ASYNC_NEW_CH_MEMBER     = 21,  // client   | public
    ASYNC_DEL_CH            = 22,  // client   | public
    ASYNC_RENAME_CH         = 23,  // client   | public
    ASYNC_CH_ACT_FLAG       = 24,  // internal | public
    ASYNC_NEW_SUB_CH        = 25,  // client   | public
    ASYNC_RM_SUB_CH         = 26,  // client   | public
    ASYNC_RENAME_SUB_CH     = 27,  // client   | public
    ASYNC_INVITED_TO_CH     = 28,  // client   | public
    ASYNC_RM_CH_MEMBER      = 29,  // client   | public
    ASYNC_INVITE_ACCEPTED   = 30,  // client   | public
    ASYNC_MEM_LEVEL_CHANGED = 31,  // client   | public
    ASYNC_SUB_CH_LEVEL_CHG  = 32,  // client   | public
    ASYNC_ADD_RDONLY        = 33,  // client   | public
    ASYNC_RM_RDONLY         = 34,  // client   | public
    ASYNC_ADD_CMD           = 35,  // client   | none
    ASYNC_RM_CMD            = 36,  // client   | none
    ASYNC_USER_RENAMED      = 37,  // internal | public
    ASYNC_PING_PEERS        = 38,  // internal | private
    ASYNC_OPEN_SUBCH        = 39,  // internal | private
    ASYNC_CLOSE_SUBCH       = 40,  // internal | private
    ASYNC_UPDATE_BANS       = 41,  // internal | private
    ASYNC_KEEP_ALIVE        = 42,  // internal | private
    ASYNC_SET_DIR           = 43,  // internal | private
    ASYNC_DEBUG_TEXT        = 44   // internal | private
};
```

### 5.2 Async Data ###

```ASYNC_RDY (1)```
This command signals to the client that your current session is now ready to start running commands. This is sent to the client after successfully setting up the tcp connection ([protocol](protocol.md)). It can carry [TEXT](type_ids.md) data that can be displayed directly to the user if needed.

```ASYNC_SYS_MSG (2)```
This command carry [TEXT](type_ids.md) or [ERR](type_ids.md) data that are system messages that can be directly displayed to the user of needed. It is also used to carry [HOST_CERT](type_ids.md) data during the tcp connection setup and MY_INFO when local user account information has changed.

```ASYNC_EXIT (3)```
This is an internal async command that doesn't carry any data. It is used to send a ```closeServer()``` signal to the TCPServer object in the main process. This will cause it stop listing for clients, close all sessions and then close the main process.

```ASYNC_CAST (4)```
This is an internal only command that carries a 54byte open sub-channels list described in section 5.3 and an embedded frame that can then be sent to clients that have any of the matching open sub-channels. It drops that sub-channel list before arriving at the client so it will apppear like a regular [mrci frame](protocol.md) of any data type.
```
from_module: [54bytes(sub_channel_list)][1byte(type_id)][rest-of-bytes(pay_load)]
to_client:   [type_id][cmd_id(4)][branch_id(0)][size_of_payload][payload]
```

```ASYNC_MAXSES (5)```
Internal only async command can used by modules to send a 4byte unsigned 32bit int to the session object to change the maximum amount the concurrent sessions for the TCPServer object.

```ASYNC_LOGOUT (6)```
This internal only async command doesn't carry any data. This can be used by modules to tell the session object to logout the current user.

```ASYNC_USER_DELETED (7)```
This command carries a 32byte user id hash of the user account that was delete from the host database. All sessions that are currently logged into this account will force logout.

```ASYNC_DISP_RENAMED (8)```
This command carries a combination of the 32byte user id hash and the 64byte new display name (UTF-16LE, padded with 0x00) of the user account that changed it's display name. This will trigger all sessions that are currently logged into this account to send an updated [MY_INFO](type_ids.md) frame via ASYNC_SYS_MSG to the clients.
```
from_module: [32bytes(user_id)][64bytes(new_disp_name)]
to_client:   [type_id(9)][cmd_id(2)][branch_id(0)][size_of_payload][payload(MY_INFO)]
```

```ASYNC_USER_RANK_CHANGED (9)```
This command carries a combination of the 32byte user id hash and 4byte new rank (32bit uint_le) of the user account that changed it's host rank. This will trigger all sessions that are currently logged into this account to send an updated [MY_INFO](type_ids.md) frame via ASYNC_SYS_MSG to the clients.
```
from_module: [32bytes(userId)][4bytes(newRank)]
to_client:   [type_id(9)][cmd_id(2)][branch_id(0)][size_of_payload][payload(MY_INFO)]
```

```ASYNC_CMD_RANKS_CHANGED (10)```
This internal only command doesn't carry any data, it just triggers all sessions to re-load runable commands.

```ASYNC_RESTART (11)```
This internal only async commmand doesn't carry any data. It is used to send a ```resServer()``` signal to the TCPServer object in the main process. This will cause it stop listing for clients, close all sessions, reload the host settings and start listening for clients again.

```ASYNC_ENABLE_MOD (12)```
This internal only async commmand that carry a [TEXT](type_ids.md) path to a module executable. All session objects that receive this will then attempt to load the module.

```ASYNC_DISABLE_MOD (13)```
This is the other half to ASYNC_ENABLE_MOD. All session objects that receive this will remove and terminate all commands associated with this module.

```ASYNC_END_SESSION (14)```
This internal only async commmand doesn't carry any data. It is used by modules to logout the current user.

```ASYNC_USER_LOGIN (15)```
This command carries a 32byte user id hash. This can be used by modules to tell the session object to login as this user.

```ASYNC_TO_PEER (16)```
This is an async command that carry an embedded data frame directly to/from peer sessions without any restrictions. It is prepended with the 224bit hash of the target session id; however, it drops that session id before arriving at the client so it will apppear as a regular mrci frame of any data type. Modules do not have direct access to this, the host internal objects will handle this.
```
from_module: [28bytes(sessionId)][1byte(typeId)][rest-of-bytes(payload)]
to_client:   [type_id][cmd_id(16)][branch_id(0)][size_of_payload][payload]
```

```ASYNC_LIMITED_CAST (17)```
This operate exactly like ASYNC_CAST except only sessions with active update sub-channels open will respond to it.

```ASYNC_RW_MY_INFO (18)```
This command carries a 32byte user id hash of a user account who's data has been overwritten by another user. This will trigger all sessions that are currently logged into this account to send an updated [MY_INFO](type_ids.md) frame via ASYNC_SYS_MSG to the clients.
```
from_module: [32bytes(user_id)]
to_client:   [type_id(9)][cmd_id(2)][branch_id(0)][size_of_payload][payload(MY_INFO)]
```

```ASYNC_P2P (19)```
This async command carry an embedded data frames directly to/from peer sessions following the p2p negotiation process as described in [Type_IDs](type_ids.md), section 3.2 at the P2P specific data types. It prepends the 28byte hash of the destination session id and source session id; however, it drops the destination id and source id for just the P2P specific data types before arriving at the client. For all other data types (if a p2p connection is estabished), the source id is prepend-moved to the payload of the mrci frame before arriving at the client.
```
from_module:     [28bytes(dst_sessionId)][28bytes(src_sessionId)][1byte(typeId)][rest-of-bytes(payload)]
to_client (P2P): [type_id][cmd_id(19)][branch_id(0)][size_of_payload][payload(src_sessionId)]
to_client:       [type_id][cmd_id(19)][branch_id(0)][size_of_payload + 28][src_sessionId + payload]
```

```ASYNC_CLOSE_P2P (20)```
This internal only async command carry the 28byte hash session id of a session that is about to close. All sessions that receive this will close the p2p connection with this session if such a connection exists. If such a connection did exist, the session will convert the frame into a P2P_CLOSE mrci frame and send it to the client via ASYNC_P2P.
```
from_module: [28bytes(src_sessionId)]
to_client:   [type_id(12)][cmd_id(19)][branch_id(0)][size_of_payload][payload(src_sessionId)]
```

```ASYNC_NEW_CH_MEMBER (21)```
This async command carries a [CH_MEMBER_INFO](type_ids.md) frame containing public information about a user that has been added to a channel member list. All sessions that are logged in as a member of the channel will forward the frame (unchanged) to the clients.
```
to_client: [type_id(25)][cmd_id(21)][branch_id(0)][size_of_payload][payload(CH_MEMBER_INFO)]
```

```ASYNC_DEL_CH (22)```
This async command carries a [CH_ID](type_ids.md) frame of a channel that has been deleted. All sessions that receive will forward the frame to the client (unchanged) and close all sub-channels related to the channel.
```
to_client: [type_id(26)][cmd_id(22)][branch_id(0)][size_of_payload][payload(CH_ID)]
```

```ASYNC_RENAME_CH (23)```
This async command carries a combination of the channel id and a 16bit null terminated UTF-16LE string containing the new name of the channel that has been renamed. This command desn't use any of the standard frame formats so it sends a [BYTES](type_ids.md) frame to the client.
```
to_client: [type_id(14)][cmd_id(28)][branch_id(0)][size_of_payload][payload(channel_id + new_channel_name)]
```

```ASYNC_CH_ACT_FLAG (24)```
This internal only async command carries a combination of the channel id and and sub-channel id to tell sessions that the sub-channel has changed it's active update flag. All sessions that have this sub-channel open will re-check this flag and determine if it should continue active updating.
```
format: [8bytes(64bit_ch_id)][1byte(8bit_sub_ch_id)]
```

```ASYNC_NEW_SUB_CH (25)```
This async command carries a comination of the channel id, sub-channel id, access level and a 16bit null terminated UTF-16LE string containing the sub-channel name when a new sub-channel is created. All sessions that are logged in as a member of the channel forwards the data to the clients as a [BYTES](type_ids.md) frame.
```
to_client: [type_id(14)][cmd_id(25)][branch_id(0)][8bytes(64bit_ch_id)][1byte(8bit_sub_ch_id)]
           [1byte(8bit_access_level)][16bit_null_term_sub-channel_name]
```

```ASYNC_RM_SUB_CH (26)```
This is the other half to ASYNC_NEW_SUB_CH. It carries just a combination of the channel id and and sub-channel id to tell all sessions that the sub-channel was deleted. All sessions that have the sub-channel open will close it.
```
to_client: [type_id(14)][cmd_id(26)][branch_id(0)][8bytes(64bit_ch_id)][1byte(8bit_sub_ch_id)]
```

```ASYNC_RENAME_SUB_CH (27)```
This async command carries a combination of the channel id, sub-channel id, access level and a 16bit null terminated UTF-16LE string containing the new sub-channel name. All sessions that are logged in as a member of the channel forwards the data to the clients as a [BYTES](type_ids.md) frame.
```
to_client: [type_id(14)][cmd_id(27)][branch_id(0)][8bytes(64bit_ch_id)][1byte(8bit_sub_ch_id)]
           [16bit_null_term_sub-channel_name]
```

```ASYNC_INVITED_TO_CH (28)```
This async command carries a [CH_MEMBER_INFO](type_ids.md) frame containing public information about a user that has been invited to a channel. All sessions that are logged in as a member of the channel will forward the frame (unchanged) to the clients.
```
to_client: [type_id(25)][cmd_id(28)][branch_id(0)][size_of_payload][payload(CH_MEMBER_INFO)]
```

```ASYNC_RM_CH_MEMBER (29)```
This async command carries a combination of the channel id and the 32byte user id of the channel member that has left or kicked from the channel member list. All sessions that are logged in as a member of the channel forwards the data to the clients as a [BYTES](type_ids.md) frame.
```
from_module: [8bytes(chId)][32bytes(userId)]
to_client:   [type_id(14)][cmd_id(29)][branch_id(0)][size_of_payload][payload(64bit_ch_id + 256bit_user_id)]
```

```ASYNC_INVITE_ACCEPTED (30)```
This async command carries a [CH_MEMBER_INFO](type_ids.md) frame containing public information about a user that has accepted an invite to a channel. All sessions that are logged in as a member of the channel will forward the frame (unchanged) to the clients.
```
to_client: [type_id(25)][cmd_id(30)][branch_id(0)][size_of_payload][payload(CH_MEMBER_INFO)]
```

```ASYNC_MEM_LEVEL_CHANGED (31)```
This async command carries a combination of the channel id, new level and the 32byte user id of the channel member that changed it's level of privilege in the channel. All sessions that are logged in as a member of the channel forwards the data to the clients as a [BYTES](type_ids.md) frame. All sessions that are currently logged in as this user also closes all sub-channels related to the channel.
```
from_module: [8bytes(chId)][32bytes(userId)][1byte(newLevel)]
to_client:   [type_id(14)][cmd_id(31)][branch_id(0)][size_of_payload][payload(64bit_ch_id + 256bit_user_id + 8bit_new_level)]
```

```ASYNC_SUB_CH_LEVEL_CHG (32)```
This async command carries a combination of the channel id and sub-channel id of a sub-channel that has changed it's level of access. All sessions that have this sub-channel open will close the sub-channel and/or all sessions that are logged in as a member of the channel forwards the data to the clients as a [BYTES](type_ids.md) frame.
```
from_module: [8bytes(chId)][1byte(subId)]
to_client:   [type_id(14)][cmd_id(32)][branch_id(0)][size_of_payload][payload(64bit_ch_id + 8bit_sub_id)]
```

```ASYNC_ADD_RDONLY (33)```
This async command carries exactly the same data as ASYNC_SUB_CH_LEVEL_CHG and has the same effect on the session sub-channels except it tells the clients that a read-only flag was added to the sub-channel.

```ASYNC_RM_RDONLY (34)```
This is the other half to ASYNC_ADD_RDONLY except if tells the clients that a read-only was removed from the sub-channel.

```ASYNC_ADD_CMD (35)```
This async command is used indirectly by modules to send [NEW_CMD](type_ids.md) frames to the session object. The session object will decide which frame gets forwarded to the client and which doesn't. Both this or ASYNC_RM_CMD sent directly to the session object does nothing. Instead, for modules called in listing mode [2.3](modules.md) (public_cmds, exempt_cmds or user_cmds); the session will detect the [NEW_CMD](type_ids.md) frames from the module and forward them to the client with this async command.
```
to_client: [type_id(16)][cmd_id(35)][branch_id(0)][size_of_payload][payload(NEW_CMD)]
```

```ASYNC_RM_CMD (36)```
This is the other half to ASYNC_ADD_CMD expect it is used to tell the client that the command was removed from the session and it carries a [CMD_ID](type_ids.md) frame instead.
```
to_client: [type_id(17)][cmd_id(36)][branch_id(0)][size_of_payload][payload(CMD_ID)]
```

```ASYNC_USER_RENAMED (37)```
This command carries a combination of the 32byte user id hash and the 48byte new user name (UTF-16LE, padded with 0x00) of the user account that changed it's user name. This will trigger all sessions that are currently logged into this account to send an updated [MY_INFO](type_ids.md) frame via ASYNC_SYS_MSG to the clients.
```
from_module: [32bytes(user_id)][48bytes(new_user_name)]
to_client:   [type_id(9)][cmd_id(2)][branch_id(0)][size_of_payload][payload(MY_INFO)]
```

```ASYNC_PING_PEERS (38)```
This internal only async commmand doesn't carry any data. It can be used by modules to request all peer sessions that have matching active update sub-channels open to return [PEER_INFO](type_ids.md) frames via ASYNC_TO_PEER directly to the session that has sent it. The session that receives these frames will in turn forward them to the client via ASYNC_LIMITED_CAST.
```
to_client: [type_id(8)][cmd_id(17)][branch_id(0)][size_of_payload][payload(PEER_INFO)]
```

```ASYNC_OPEN_SUBCH (39)```
This internal only async command carries a combination of the channel id and sub-channel id. it can be used by modules to tell the session object open a sub-channel.
```
format: [8bytes(64bit_ch_id)][1byte(8bit_sub_ch_id)]
```

```ASYNC_CLOSE_SUBCH (40)```
This is the other half to ASYNC_OPEN_SUBCH that tells the session to close the requested sub-channel.

```ASYNC_UPDATE_BANS (41)```
This internal only async command doesn't carry any data. It can be use by modules to tell the TCPServer object to update it's ip ban list cache from the database. This generally only needs to be used if the ip ban list in the database has changed in anyway.

```ASYNC_KEEP_ALIVE (42)```
This internal only async command doesn't carry any data. The session object normally sends a [KILL_CMD](type_ids.md) to the module when it detects that the module process has not sent an IPC frame in 2 minutes to terminate the module process. If desired, the module can send this async command in regular intervals to reset this 2 minute idle timer to prevent auto termination.

```ASYNC_SET_DIR (43)```
This internal only async command carries a [TEXT](type_ids.md) path that sets the working directory for the session object. All module processes started by the session will use this directory as the working directory and it is not shared among peer sessions. nothing happens if the path is invalid or does not exists.

```ASYNC_DEBUG_TEXT (44)```
This internal only async command carries a [TEXT](type_ids.md) debug message to be logged into the host debug log from the module. Modules can use this to help with debugging issues if it doesn't have direct access to the host database.

### 5.3 Open Sub-Channel List ###

An open sub-channel list is a binary data structure that string togeather up to 6 channel-sub combinations that indicate which channel id and sub id combinations are currently open. Each sub-channel are 9bytes long and the list itself maintians a fixed length of 54bytes so it is padded with 0x00 chars to maintain the fixed length (this padding can appear anywhere in 9byte increments within the list). Each sub-channel is formatted like this:

```
  bytes[0-7] - 64bit LE unsigned int (channel id)
  bytes[8]   - 8bit LE unsigned int (sub id)

  note: channel id 0 is not a valid id and the sub id cannot
        be valid without a valid channel id.
```
