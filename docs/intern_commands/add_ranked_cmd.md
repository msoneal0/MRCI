### Summary ###

assign a rank to a module's command name.

### IO ###

```[-mod (text) -command (text) -rank (int)]/[text]```

### Description ###

this will assign the command rank in -rank to the command name given in -command for the module executable given in -mod. assigning ranks to commands give users that have that rank or higher access to running the command. any commands that don't have an assigned rank is assumed a rank of 1. the lower the numeric value of the rank, the higher the rank (0 is invalid). some commands can claim immunity from host ranking which basically means the command is allowed to load/run regardless of the user's host rank. for internal commands, any commands that edit the user's own information like it's name, display name, password, etc including the login command have immunity. the path to the mod executable can be relative or absolute but this command doesn't check if it actually exists. for ranked command enforcement to work properly, the mod to command name relationship must match with what is actually there.