### Summary ###

move/rename a file or directory in the host file system.

### IO ###

```[-src (text) -dst (text) {-force}]/[text]```

### Description ###

move/rename the file system object (file/directory/symlink) given in -src to the destination given in -dst. this command will ask a confirmation question of the the destination file already exists; pass -force to bypass this.