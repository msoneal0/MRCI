### Summary ###

remove a read only flag from a certain sub-channel privilege level combination.

### IO ###

```[-ch_name (text) -sub_id (int) -level (int)]/[text]```

### Description ###

remove a read only flag the sub-channel given in -sub_id for connected users at the level given in -level. the channel given in -ch_name must already exists. with, the read only flag gone users connected to the sub-channel at the specified level would be able to cast data to the sub-channel once again. also note that this command will cause all sessions that currently have the sub-channel open to close the sub-channel. it will be up to the clients to re-open the sub-channel.