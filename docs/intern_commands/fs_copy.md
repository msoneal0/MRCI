### Summary ###

copy a file or directory in the host file system.

### IO ###

```[-src (text) -dst (text) {-force}]/[text]```

### Description ###

copy the file system object (file/directory/symlink) given in -src to the destination given in -dst. this command will ask a confirmation question of the the destination object already exists; pass -force to bypass this. this will also do a full recursive copy if copying a directory. 