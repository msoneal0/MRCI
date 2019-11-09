### Summary ###

create a new host user account.

### IO ###

```[-name (text) -email (text) {-disp (text)}]/[text]```

### Description ###

this creates a new user account with the user name given in -name and an email address used for account recovery in -email. the command will fail if the user name or email address already exists. you can pass the optional -disp to set the display name for the new user account. the display name can be used by clients to present the user account to other clients without showing the true user name or make it easier for users to identify each other since the display name is not restricted by uniqueness. the display name can be anything; it's only restricted to 32 chars or less but it cannot contain new lines or lines breaks.