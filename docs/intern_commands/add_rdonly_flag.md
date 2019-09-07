### Summary ###

add a read only flag to a certain sub-channel and privilege level.

### IO ###

```[-ch_name (text) -sub_id (int) -level (int)]/[text]```

### Description ###

this adds a read only flag to a sub-channel for users at a certain privilage level given in -level. valid privilage levels range 1-5 representing owner-level(1), admin-level(2), officer-level(3), regular-level(4) and public-level(5). the presents of a read only flag bassically tells the host that users connected to this sub-channel at this level can only receive data from this sub-channel and cannot cast data to the sub-channel. this is useful if you want to setup a channel that allow certain user(s) to broadcast data while everybody else can only listen for the data. the channel name given in -ch_name must already exists but the sub-channel id given in -sub_id doesn't need to exists (valid range 0-255) but what ever sub-channel name that takes the sub-id specified here gets the read only flag. only the channel owner-level(1) and admin-level(2) can add read only flags to sub-channels. also note that this command will cause all sessions that currently have the sub-channel open to close it. it will be up to the clients to re-open the sub-channel.