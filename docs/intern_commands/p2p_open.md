### Summary ###

accept the p2p request you may have received from another client connected to the host.

### IO ###

```[SESSION_ID]/[none]```

### Description ###

send out a P2P_OPEN to the client's 28byte (224bit) session id given to this command. use this in response to a P2P_REQUEST to accept it. this command fails if you did not receive a P2P_REQUEST from the peer client.