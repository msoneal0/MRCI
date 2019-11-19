### 4.1 Host Features ###

Other than transporting data to be processed by modules, the host have a few other built in features such as data broadcasting to/from clients connected to the host and full user account management. The following concepts needed to be created to facilitate these specific features:

### 4.2 Host Ranks ###

Each user registered in the host must be assigned a host rank. The lower it's numeric value, the higher the level of access the user has in the host with 0 being invalid. By default, the host defines the ```root``` user with a host rank of 1; giving it the highest level of access possible in the host. The host also defines a default initial host rank of 2 for all new accounts that are created in the host. This can be re-configured at any time using the [host_config](intern_commands.md) internal command.

Some internal commands have the ability to change the user account information of other accounts. The host will in general not allow users of a lower level of access to any user information of higher access level. For example: a user of host rank ```1``` can force change another user's email address if that user's rank is ```2``` or higher but the rank ```2``` user can't do the same in reverse.

Host ranks can also be assigned to the commands themselves via the command names of specific modules. By doing this, the host can be configured to allow users of certain ranks or higher access to running certain commands. For example: if a command named ```this_cmd``` is assigned a host rank of ```6```, all users with a host rank value of ```6``` or lower will have access to running this command. All commands that don't have an assigned rank will be assumed a rank of ```1``` but all commands that define itself as rank exempt can bypass this and allow the user to run it regardless of the user's host rank. Exemptions would also disregard the assigned rank of the command.

### 4.3 Channels And Sub-channels ###

Channels are used to manage sub-channels and sub-channels are used to determine which clients can send/receive broadcast data. When a client broadcast data to a open sub-channel, the clients that want to receive that data will also need to have the matching sub-channel open. Channels and sub-channels can be identified by a string name or integer id. 

When a channel is created, the user only needs to provide a unique channel name because the host will auto generate a unique channel id attached to that name. This id is a 64bit unsigned integer and it remains constant for as long as the channel exists, even when the name changes. The channel however can't broadcast any data until a sub-channel is created with the user providing a sub-channel name unique to the channel and the host will auto generate a sub-channel id also unique to the channel. The sub-id is a 8bit unsigned integer and it also remains constant for as long as the sub-channel exists. 

When brocasting data, a combination of the channel and sub-channel ids are used as described in [ASYNC_CAST](async.md) or [ASYNC_LIMITED_CAST](async.md).

Access to opening sub-channels are managed in a way similar to the host ranks. All channels have a list of host user accounts that are members of the channel, each having managed levels of access to what they can do as a member of the channel. Host users are added via invitation and the users themselves have the option to accept or decline the invitation to join the channel. Like the host rank, access levels to the channels are a numeric integer value where the lower it's value is the higher level of access the channel member will have but unlike the host rank, the access levels are not user defined. Instead, all levels are host defined and what each level can do is also host defined.

```
enum ChannelMemberLevel
{
    OWNER   = 1,
    ADMIN   = 2,
    OFFICER = 3,
    REGULAR = 4,
    PUBLIC  = 5
};
```

Below is a description of what each of these levels can do within the channel:

owner-level(1):

1. delete or rename the channel.
2. delete, create or rename sub-channels within the channel.
3. invite new users to the channel or cancel invites.
4. remove any member of the channel except your self.
5. set sub-channels' level of access.
6. add/remove read only flags to/from sub-channels.
7. update the privilege level of any member in the channel.
8. open/close sub-channels.

admin-level(2):

1. delete, create or rename sub-channels within the channel.
2. invite new users to the channel or cancel invites.
3. remove any member of channel except the owner and other admins.
4. set sub-channels' level of access.
5. add/remove read only flags to/from sub-channels.
6. update the privilege level of members up to your own level but not above.
7. open/close sub-channels.

officer-level(3):

1. invite new users to the channel or cancel invites.
2. can only remove regular members of the channel.
3. update the privilege level of members up to your own level but not above.
4. open/close sub-channels.

regular-level(4):

1. open/close sub-channels.

public-level(5):

1. open/close public sub-channels.

All sub-channels can be configured with it's own "lowest level of access" level that can make it so only certain channel members can open it. For example, the channel owner or admin can set a sub-channel's minimum level to 4 to make it so only channel regular members and above can open the sub-channel or it can be set to 5 to make it so anybody can open/close the sub-channel, affectively making it a public sub-channel.

There can only be one channel owner so if the owner decides change the privilege of any other member in the channel to owner, the current owner will automatically step down to level 2 (admin). Also note that privilege level 5 is reserved for users that are not a member of the channel and cannot be assigned to any user that are currently members of the channel.

Sub-channels can also be assigned what is called *read-only* flags. These flags are attached to the sub-channel id and privilege level. It is decoupled from all changes that could occur with the sub-channel so this means the sub-channel can get renamed or even deleted but the read-only flag would still remain. What a read-only flag actual does is make it so certain users of the matching level can listen for broadcast data but cannot send out broadcast data to the sub-channel. So a read-only flag for example can be added to a sub-channel id for privilege 5 to make it so public users can listen to the sub-channel but cannot send out anything to it.