### Summary ###

create a new sub-channel within a channel.

### IO ###

```[-ch_name (text) -sub_name (text)]/[text]```

### Description ###

create a new sub-channel given in -sub_name for the channel given in -ch_name. only the channel owner-level(1) and admin-level(2) is allowed to do this. sub-channels are used to determine which clients can send/receive broadcast data. when a client broadcast data to a open sub-channel, the clients that want to receive that data will also need to have the matching sub-channel open. 