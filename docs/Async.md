### 6.1 Async Commands ###

An async command is a virtual command that the host can use to send data to the client at any time while connected to the host. As the name implies, the occurance of a client receiving data from an async command is not always the result of running a regular command in the current session. This can occur for example when information in your account is changed by another client connected to the host; your client would not know about this change until an async command is sent notify it of the change. These commands cannot be called directly by a client or even a module command object.

Async commands are not only used send data to the client but also used internally within the host to help session objects operating in different processes to communicate with each other. Some async commands in fact are considered internal only because the client should never see any data come from them at anytime.

These are considered "virtual" commands because there is no defined command objects attached to them. Instead, async commands are best identified by command id values 1-255. Here is a list of currently "defined" async commands:

```
#define ASYNC_RDY                1
#define ASYNC_SYS_MSG            2
#define ASYNC_EXE_CRASH          3
#define ASYNC_EXIT               4   // internal only
#define ASYNC_CAST               5   // internal only
#define ASYNC_MAXSES             6   // internal only
#define ASYNC_LOGOUT             7   // internal only
#define ASYNC_USER_DELETED       8   // internal only
#define ASYNC_GROUP_RENAMED      9   // internal only
#define ASYNC_DISP_RENAMED       10  // internal only
#define ASYNC_GRP_TRANS          11  // internal only
#define ASYNC_USER_GROUP_CHANGED 12  // internal only
#define ASYNC_CMD_RANKS_CHANGED  13  // internal only
#define ASYNC_RESTART            14  // internal only
#define ASYNC_ENABLE_MOD         15  // internal only
#define ASYNC_DISABLE_MOD        16  // internal only
#define ASYNC_GROUP_UPDATED      17  // internal only
#define ASYNC_END_SESSION        18  // internal only
#define ASYNC_USER_LOGIN         19  // internal only
#define ASYNC_RESTORE_AUTH       20  // internal only
#define ASYNC_TO_PEER            21
#define ASYNC_LIMITED_CAST       22
#define ASYNC_RW_MY_INFO         23  // internal only
#define ASYNC_P2P                24
#define ASYNC_CLOSE_P2P          25  // internal only
#define ASYNC_NEW_CH_MEMBER      26
#define ASYNC_DEL_CH             27
#define ASYNC_RENAME_CH          28
#define ASYNC_CH_ACT_FLAG        29
#define ASYNC_NEW_SUB_CH         30
#define ASYNC_RM_SUB_CH          31
#define ASYNC_RENAME_SUB_CH      32
#define ASYNC_INVITED_TO_CH      33
#define ASYNC_RM_CH_MEMBER       34
#define ASYNC_INVITE_ACCEPTED    35
#define ASYNC_MEM_LEVEL_CHANGED  36
#define ASYNC_SUB_CH_LEVEL_CHG   37
#define ASYNC_ADD_RDONLY         38
#define ASYNC_RM_RDONLY          39
#define ASYNC_ADD_CMD            40
#define ASYNC_RM_CMD             41
#define ASYNC_USER_RENAMED       42
#define ASYNC_PUBLIC_AUTH        43  // internal only
```

### 6.2 Async Data ###

```ASYNC_RDY (1)```
This command signals to the client that your current session is now ready to start running commands. This is usually sent after successfully setting up the tcp connection ([protocol](Protocol.md)) or after successfully recovering from a session crash. It can carry ```TEXT``` data that can be displayed directly to the user if needed.

```ASYNC_SYS_MSG (2)```
This command carry ```TEXT``` or ```ERR``` data that are system messages that can be directly displayed to the user of needed. It is also used to carry ```HOST_CERT``` data during the tcp connection setup and ```MY_INFO``` when local user account information has changed.

```ASYNC_EXE_CRASH (3)```
This is used to send ```ERR``` messages to the client if your session crashes or fails to setup an IPC connection for any reason.

```ASYNC_EXIT (4)```
This is an internal async command that doesn't carry any data. It is used to send a ```closeServer()``` signal to the ```TCPServer``` object in the main process. This will cause it stop listing for clients, close all sessions and then close the main process.

```ASYNC_CAST (5)```
This is an internal only command that carries a 54byte open sub-channels list ```wrAbleChIds``` described in section [3.4](Command_Objects.md) and an embedded mrci frame that can then be sent to clients that have any of the matching open sub-channels. It drops that sub-channel list before arriving at the client so it will apppear like a regular mrci frame of any data type.

```ASYNC_MAXSES (6)```
Internal only async command that is used by internal commands to send a ```BYTES``` frame to the main process to update the maximum amount the concurrent sessions for the ```TCPServer``` object. The data itself is actually a 32bit unsigned int.

```ASYNC_LOGOUT (7)```
This internal only async command doesn't carry any data. This is just used to notify the main process ```Session``` object that the user has logged out and should not attempt to restore authentication in case of a session crash. This doesn't actually do the logout.

```ASYNC_USER_DELETED (8)```
This internal only async command carry ```TEXT``` data that is the user name of the user account that was deleted from the host database. All ```Session``` objects that get this command must read and match this to the user name that is currently logged in for that object. If the user name matches, the ```Session``` object must logout since the user account no longer exists.

```ASYNC_GROUP_RENAMED (9)```
Internal only async command that carry ```TEXT``` command line arguments to notify all ```Session``` objects that the a host group name has changed. Example: ```-src "old_group_name" -dst "new_group_name"```. All ```Session``` objects that have matching current group names to ```old_group_name``` must update that group name to ```new_group_name``` and also send a ```ASYNC_SYS_MSG``` containing an updated ```MY_INFO```.

```ASYNC_DISP_RENAMED (10)```
Internal only async command that carry ```TEXT``` command line arguments to notify all ```Session``` objects that a user has changed the display name. Example: ```-name "new_display_name" -user "user_name"```. All ```Session``` objects that have matching current user names to ```user_name``` must update the display name to ```new_display_name``` and also send a ```ASYNC_SYS_MSG``` containing an updated ```MY_INFO```.

```ASYNC_GRP_TRANS (11)```
Internal only async command that carry the same ```TEXT``` command line arguments as ```ASYNC_GROUP_RENAMED``` to notify all ```Session``` objects that the all users currently in the group given in ```-src``` must be updated to the group given in ```-dst```. This triggers a ```ASYNC_SYS_MSG``` to send an updated ```MY_INFO``` and the ```Session``` object will load all the commands that the user now have access to and remove any commands that lost access due to the group change.

```ASYNC_USER_GROUP_CHANGED (12)```
This is an internal only command carry ```TEXT``` command line arguments to notify all ```Session``` objects that a user's group was changed to another group. example: ```-user "user_name" -group "new_group"```. All ```Session``` objects the have the matching user name need to update it's group to ```new_group```, send a ```ASYNC_SYS_MSG``` with an updated ```MY_INFO``` and load all the commands that the user now have access to and remove any commands that lost access due to the group change.

```ASYNC_CMD_RANKS_CHANGED (13)```
This internal only async commmand doesn't carry any data. Instead, it notifies all ```Session``` objects that assigned command ranks have changed. This will cause all of the ```Session``` objects to re-check the currently loaded commands and determine if it needs to remove any commands that the user no longer have access to and add any commands that the user may have gained access to.

```ASYNC_RESTART (14)```
This internal only async commmand doesn't carry any data. It is used to send a ```resServer()``` signal to the ```TCPServer``` object in the main process. This will cause it stop listing for clients, close all sessions, reload the host settings and start listening for clients again.

```ASYNC_ENABLE_MOD (15)```
This internal only async commmand carry ```TEXT``` of a module name. All ```Session``` objects that receive this will then load the requested module.

```ASYNC_DISABLE_MOD (16)```
This internal only async commmand carry ```TEXT``` of a module name. All ```Session``` objects that receive this will delete all commands associated with the with this module and then unload it.

```ASYNC_GROUP_UPDATED (17)```
This is an internal only command that carry ```TEXT``` command line arguments to notify all ```Session``` objects that a group's host rank has changed. Example: ```-name "group_name" -rank 2```. All ```Session``` object that have matching group names to ```group_name``` will need to update the host rank to ```2```. When the session's host rank to adjusted this way, the session will need to re-check the currently loaded commands and determine if it needs to remove any commands that the user no longer have access to and add any commands that the user may have gained access to.

```ASYNC_END_SESSION (18)```
This internal only async commmand doesn't carry any data. It is used to notify the main process that the ```CmdExecutor``` object is requesting to close the session. The main process ```Session``` object will then signal the slave process to close and close the tcp session.

```ASYNC_USER_LOGIN (19)```
This is an internal only command that carry ```PRIV_IPC``` data containing the 256bit Keccak hash user id of the user that has successfully logged in. This is used by the ```CmdExecutor``` object to notify the main process ```Session``` object of the current user associated with the current session. The main process ```Session``` object will use this information to restore the user authorization in case of a session crash.

```ASYNC_RESTORE_AUTH (20)```
This is an internal only command that carry ```PRIV_IPC``` data containing the 256bit Keccak hash user id of a user that has successfully logged in with the current session. It is used by the ```Session``` object in the main process to tell the ```CmdExecutor``` object in the slave process to authorize the user without a password. This is only used when attempting to restore the session from a crash and a user was logged in at the time.

```ASYNC_TO_PEER (21)```
This is an async command that carry an embedded mrci frame directly to/from peer sessions without any restrictions. It is prepended with the 224bit sha3 hash of the target session id; however, it drops that session id before arriving at the client so it will apppear as a regular mrci frame of any data type. Users should not be given direct access to this for security reasons.

```ASYNC_LIMITED_CAST (22)```
This operate exactly like ```ASYNC_CAST``` except only sessions with active update sub-channels will respond to it.

```ASYNC_RW_MY_INFO (23)```
This internal only async command carry ```TEXT``` data of the user name to tell all ```Session``` objects that have the matching user name to send an update ```MY_INFO``` to the client. This is useful for when a host admin force updates user information of other lesser privileged users to make their clients aware of the changes.

```ASYNC_P2P (24)```
This async command carry an embedded mrci frame directly to/from peer sessions following the p2p negotiation process as described in [Type_IDs](Type_IDs.md), section 4.2 at the P2P specific data types. It prepends the 224bit sha3 hash of the destination session id and source session id; however, it drops the destination id and source id for just the P2P specific data types before arriving at the client. For all other data types (if a p2p connection is estabished), the source id is prepend-moved to the payload of the mrci frame before arriving at the client.

```ASYNC_CLOSE_P2P (25)```
This internal only async command carry a 224bit sha3 hash session id of a session that is about to close. This notifies all ```CmdExecutor``` objects that have matching hashes in ```p2pAccepted``` and ```p2pPending``` (section [3.4](Command_Objects.md)) to remove them now. It also triggers a ```ASYNC_P2P``` to send a ```P2P_CLOSE``` for the session id in question so the clients can also be made aware of this.

```ASYNC_NEW_CH_MEMBER (26)```
```TEXT``` command line arguments when a new channel is created and the user that created it is added as the channel owner. Example: ```-user "user_name" -ch_name "new_channel" -ch_id 334 -level 1```.

```ASYNC_DEL_CH (27)```
```TEXT``` command line arguments when a channel is deleted. Example: ```-ch_name "channel_name" -ch_id 426```. The host will automatically close all sub-channels related to this channel for all sessions that have them open.

```ASYNC_RENAME_CH (28)```
```TEXT``` command line arguments when a channel is renamed. Example: ```-ch_name "old_name" -new_name "new_name"```.

```ASYNC_CH_ACT_FLAG (29)```
```TEXT``` command line arguments when sub-channel's active update flag has been updated. Example: ```-ch_name "channel_name" -sub_name "sub_channel" -state 1```. (```-state``` 1 is true or ```-state``` 0 is false).

```ASYNC_NEW_SUB_CH (30)```
```TEXT``` command line arguments when a new sub-channel is created. Example: ```-ch_name "channel_name" -sub_name "new_sub_channel" -ch_id 987 -sub_id 5 -level 2```.

```ASYNC_RM_SUB_CH (31)```
```TEXT``` command line arguments when a sub-channel is deleted. Example: ```-ch_name "channel_name" -sub_name "sub_channel" -ch_id 987 -sub_id 5```. The host will automatically close this sub-channel for sessions that currently have it open.

```ASYNC_RENAME_SUB_CH (32)```
```TEXT``` command line arguments when a sub-channel is renamed. Example: ```-ch_name "channel_name" -sub_name "sub_channel" -new_name "new_sub_name"```.

```ASYNC_INVITED_TO_CH (33)```
```TEXT``` command line arguments when a new user is invited to join a channel. Example: ```-ch_name "channel_name" -user "user_name"```.

```ASYNC_RM_CH_MEMBER (34)```
```TEXT``` command line arguments when a user is kicked from a channel, uninvited or has left the channel. Example: ```-ch_name "channel_name" -user "user_name" -ch_id 746```.

```ASYNC_INVITE_ACCEPTED (35)```
```TEXT``` command line arguments when a user that was previously invite to join the channel has accepted the invite and should now be considered full member of the channel starting off at level ```REGULAR```. Example: ```-ch_name "channel_name" -user "user_name"```.

```ASYNC_MEM_LEVEL_CHANGED (36)```
```TEXT``` command line arguments when a channel member's privilege level is changed. Example: ```-ch_name "channel_name" -user "user_name" -ch_id 774 -level 2```. The host automatically closes all sub-channels related to the channel for the affected user. It will be will up to the client to re-open the sub-channel(s) if the user still have access to it/them.

```ASYNC_SUB_CH_LEVEL_CHG (37)```
```TEXT``` command line arguments when a sub-channel's lowest level of access is changed. Example: ```-ch_name "channel_name" -sub_name "sub_channel" -level 3 -ch_id 645 -sub_id 5```. The host will automatically close this sub-channel for sessions that currently have it open. It will be up to the client to reopen it if the current user still have access to it.

```ASYNC_ADD_RDONLY (38)```
```TEXT``` command line arguments when a read only flag is added to a sub-channel's access level. Example: ```-ch_name "channel_name" -sub_id 5 -level 4```. The host will automatically close this sub-channel for sessions that currently have it open. It will be up to the client to reopen it if the current user still have access to it.

```ASYNC_RM_RDONLY (39)```
```TEXT``` command line arguments when a read only flag is removed from a sub-channel's access level. Example: ```-ch_name "channel_name" -sub_id 5 -level 4```. The host will automatically close this sub-channel for sessions that currently have it open. It will be up to the client to reopen it if the current user still have access to it.

```ASYNC_ADD_CMD (40)```
This async command carry ```NEW_CMD``` when the session's ```CmdExecutor``` loads a new command object and wants to notify the client of it.

```ASYNC_RM_CMD (41)```
This async command carry ```CMD_ID``` when the session's ```CmdExecutor``` deletes a command object and wants to notify the client of it.

```ASYNC_USER_RENAMED (42)```
```TEXT``` command line arguments when a user changes it's user name. Example: ```-old "old_user_name" -new_name "new_user_name"```.

```ASYNC_PUBLIC_AUTH (43)```
This internal only async commmand doesn't carry any data. It just tells the ```CmdExecutor``` to load or reload commands without an authorized user so only public commands will be available until the client authorizes into a user account. This is used by the host when starting a session for the first time or if restoring the session after a crash and no user was logged in at the time.