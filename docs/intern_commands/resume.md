### Summary ###

resumes the current task that the command is running.

### IO ###

```[{CMD_ID}]/[text]```

### Description ###

this resumes the looping task for the command given by a CMD_ID frame sent into this command. a failure is returned if the command is not currently in a paused state. if the frame is empty, all currently paused commands are resumed.