### Summary ###

display or change the current directory for the current session.

### IO ###

```[{-path (text)}]/[text]```

### Description ###

change the current directory of your current session to the directory specified in -path or display it if -path is not specified. note: the current directory is not shared among any of the peer sessions and changes to it does not take effect on currently running commands until fully terminated (not IDLE but fully terminated via KILL_CMD).