### Summary ###

open a sub-channel to send/receive broadcasted data to/from peers.

### IO ###

```[-ch_name (text) -sub_name (text)]/[text]```

### Description ###

use this command to open the channel and sub-channel name given in -ch for the main channel name and -sub for the sub-channel name. only peers with matching channel-sub combinations can send/receive data with each other when using the cast command. whether peers can send/receive PEER_INFO or PEER_STAT frames depends if the active update flag is set on the sub-channel itself or the host global setting.