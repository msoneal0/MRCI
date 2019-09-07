### Summary ###

create a new channel.

### IO ###

```[-ch_name (text)]/[text]```

### Description ###

create a new channel with a unique name given in -ch_name. this command will automatically add the currently logged in user as a member and set it as the channel owner. being a channel owner prevents you from being able to delete your account at a later time. you will need to name a new owner from the channel member list or delete the entire channel to allow account deletion. the host will automatically assign a unique channel id for the new channel. channels are used for broadcast peer to peer data transfers within the host and the base channel created by this command is used to manage who has access to such data.