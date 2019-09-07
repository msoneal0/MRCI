### Summary ###

set or unset the active update flag of a sub-channel.

### IO ###

```[-ch_name (text) -sub_name (text) -state (1 or 0)]/[text]```

### Description ###

this sets the active update flag on the channel given in -ch_name true or false based on the value given in -state. the active update flag allow the sessions to send PEER_INFO or PEER_STAT frames to the clients when a peer connected to the channel changes information like it's user name, group name, display name, disconnect etc...this flag is ignored if the host have the global active update flag set. if that's the case, all channels will active update. the channel owner-level(1) and admin-level(2) is allowed to do this.