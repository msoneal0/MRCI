### Summary ###

set/unset the channel owner override flag for your current session.

### IO ###

```[-state (1 or 0)]/[text]```

### Description ###

This sets/unsets the channel owner override flag for your current session according to the bool value passed into -state. the owner override flag basically tells the host that the current session can do anything a channel owner can do to any channel in the host and doesn't even need to be a member. this is a simple but very powerful command, it is best reserved for host administrators only.