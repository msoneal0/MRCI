### Summary ###

close the p2p connection with the client given in this command or decline a p2p request.

### IO ###

```[SESSION_ID]/[none]```

### Description ###

send out a P2P_CLOSE to the client's 28byte (224bit) session id given to this command. this can be used to decline a P2P_REQUEST or close the p2p connection you may have the peer. this command fails if you have no such connection or no pending P2P_REQUEST with the peer.