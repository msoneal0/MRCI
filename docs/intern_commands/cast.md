### Summary ###

broadcast text/data to all sessions listening to any matching sub-channels.

### IO ###

```[any]/[any]```

### Description ###

this sends data passed into it to all peers connected to any of your matching opened sub-channels. this is useful for broadcasting data session-to-session. nothing gets re-sent to the session that initiated the cast.