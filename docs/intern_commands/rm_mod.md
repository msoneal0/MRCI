### Summary ###

remove a module from the host.

### IO ###

```[-mod_path (text)]/[text]```

### Description ###

this tells all sessions in the host to unload the module given in -mod_path and all associated commands. all associated commands are forced to terminate when the module is unloaded. this command will not uninstall the module application, all it does is remove the path to the module application from the host database so the module's commands will no longer be executable by host users.