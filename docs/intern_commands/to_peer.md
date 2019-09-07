### Summary ###

send/receive any data directly with a client connected to the host that has accepted your p2p request or the peer's p2p request.

### IO ###

```[(SESSION_ID)any]/[none]```

### Description ###

send any type of data directly to a peer client connected to the host that has accepted your p2p request. you must prepend the data sent into this command with the 224bit hash the peer's session id. an error is called out if the peer session has not accepted your p2p request.