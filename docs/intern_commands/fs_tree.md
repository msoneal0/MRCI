### Summary ###

list all files and sub-directories of an entire directory tree.

### IO ###

```[{-path (text)} {-info_frame} {-no_hidden}]/[text] or [FILE_INFO]```

### Description ###

this list all files and sub-directories in the entire tree of the current directory or the directory specified in -path. this command normally returns human readable text for each file or sub-directory that is listed but you can pass -info_frame to make the command return FILE_INFO frames for each file/sub-directory instead. note: if displaying as text, all directory names are displayed with a '/' at the end. by default, this command will list all hidden files and directories among the visible but you can pass the -no_hidden option to have it not list the hidden files or directories.