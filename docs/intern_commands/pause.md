### Summary ###

pause the current task that the command is running.

### IO ###

```[{CMD_ID}]/[text]```

### Description ###

this pauses the command given by a CMD_ID frame sent into this command. an error is returned if the requested command is not currently in the looping state. if the frame is empty, this command pauses all commands currently in the looping state.