### Summary ###

set the user privilege levels of a channel member.

### IO ###

```[-ch_name (text) -user (text) -level (int)]/[text]```

### Description ###

set the privilege level of the channel member given in -user for the channel given in -ch_name to the level given in -level. a valid privilege level is an integer between 1-4 representing owner-level(1), admin-level(2), officer-level(3) and regular-level(4). the channel owner-level(1) is reserved for just 1 member in the channel and only that member can assign another member in the channel that privilege level. when the owner privilege is assigned to another member, the current owner will step down to a admin. this command is restricted based on your current level so you cannot assign a level higher that you own. below is a description of what each of these levels can do within the channel:

owner-level(1):
1. delete or rename the channel.
2. delete, create or rename sub-channels within the channel.
3. invite new users to the channel or cancel invites.
4. remove any member of the channel except your self.
5. set sub-channels' level of access.
6. add/remove read only flags to/from sub-channels.
7. update the privilege level of any member in the channel.
8. open/close sub-channels for casting.

admin-level(2):
1. delete, create or rename sub-channels within the channel.
2. invite new users to the channel or cancel invites.
3. remove any member of channel except the owner and other admins.
4. set sub-channels' level of access.
5. add/remove read only flags to/from sub-channels.
6. update the privilege level of members up to your own level but not above.
7. open/close sub-channels for casting.

officer-level(3):
1. invite new users to the channel or cancel invites.
2. can only remove regular members of the channel.
3. update the privilege level of members up to your own level but not above.
4. open/close sub-channels for casting.

regular-level(4):
1. open/close sub-channels for casting.

note: this command causes all sessions that are loggined in as this user to close all related sub-channels. it will be up to the client to re-open the sub-channels if the user still have access to them.