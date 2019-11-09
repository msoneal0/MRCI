### Summary ###

remove a user as a member of a channel you currently a member of or cancel an invite.

### IO ###

```[-ch_name (text) -user (text)]/[text]```

### Description ###

remove a user given in -user from the channel member list for the channel given in -ch_name. normally, only the channel owner-level(1), admin-level(2) of officer(3) can do this but it is unrestricted if removing your self as a member except the channel owner. another restriction is if trying to remove members with higher or equal member privileges than your self so officers can't remove admins, other officers or the owner and admins can't remove other admins or the owner. the owner can't be removed from the channel in any case. this command can also be used to cancel an invite for a user.