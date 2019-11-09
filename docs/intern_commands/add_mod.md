### Summary ###

add a new module to the host.

### IO ###

```[-mod_path (path)]/[text]```

### Description ###

add a new module to the host in the form of a path to an executable file given in -mod_path. the path can be absolute, relative to the host working directory or can contain environmental variables. a module that gets successfully added to the database via this command does not guarantee it's commands will successfuly load. any failures during module loading will be logged in the host debug log.