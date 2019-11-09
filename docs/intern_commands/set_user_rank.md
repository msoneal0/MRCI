### Summary ###

change a user account's host rank.

### IO ###

```[-user (text) -rank (int)]/[text]```

### Description ###

this changes the host rank of the target user account name given in -user to the rank given in -rank. the host rank itself is an integer value that determine what commands each user can or cannot run depending on how the ranked commands are configured. it is also used to determine what direct changes a user can do to another user's account. the lower the numerical value the rank is the higher level of privilege the user have on the host (zero is invalid so 1 is the highest level of privilege any user can have).