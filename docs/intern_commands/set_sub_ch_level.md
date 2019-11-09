### Summary ###

set the lowest privilege level that members need to be in order to open a certain sub-channel.

### IO ###

```[-ch_name (text) -sub_name (text) -level (int)]/[text]```

### Description ###

this command makes it possible so set minimum privilege levels to open the sub-channel given in -sub_name to the level given in -level for the channel given in -ch_name. a valid level is an integer between 1-5 representing owner-level(1), admin-level(2), officer-level(3), regular-level(4) and public-level(5). for example, you could set a sub-channel's minimum level to 4 to make it so only channel regular members and above can open/close the sub-channel or you can set it to 5 to make it so anybody can open/close the sub-channel, affectively making it a public sub-channel. only the channel owner or admin(s) are allowed to update this. also note that this command will cause all sessions that currently have the sub-channel open to close the sub-channel. it will be up to the client to re-open the sub-channel if it's user account still have access to it.