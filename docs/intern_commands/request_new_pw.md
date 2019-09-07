### Summary ###

enable/disable a password change request for a user on next login.

### IO ###

```[-user (text) -req (0 or 1)]/[text]```

### Description ###

this will set or unset the request for the user given in -user to change the password on next login. pass 0 on -req to disable the request or pass 1 to enable the request. the request is automatically disabled when the user successfully changes the password.