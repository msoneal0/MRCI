### Summary ###

uninstall a module from the host.

### IO ###

```[-name (text)]/[text]```

### Description ###

this tells all sessions in the host to unload the module given in -name and all associated commands. it will also permanently delete all files associated with the module from the host. file deletion is not done instantly; instead, a specific amount of time is allotted to allow all sessions to unload the mod/clean up before it's files are deleted. also note that all associated commands are forced to terminate when the module is unloaded.