### Summary ###

attempt to delete a file system object (file,directory,symlink) in the host.

### IO ###

```[-path (text) {-force}]/[text]```

### Description ###

attempt to delete the file system object (file/directory/symlink) given in -path. pass -force to bypass the command's confirmation question. note: this will do a recursive delete if deleting a directory.