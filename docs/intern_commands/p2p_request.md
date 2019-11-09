### Summary ###

send out a p2p request to the peer session id given in this command.

### IO ###

```[SESSION_ID]/[P2P_CLOSE] or [P2P_OPEN]```

### Description ###

send out a P2P_REQUEST to another client connected to the host. a P2P_OPEN frame will be returned to you from the target session if the request is accepted. you will then be allowed to send any data to/from this peer. a P2P_CLOSE is returned if the request is declined. this command simply takes a 28byte (224bit) hash of the peer client's session id.