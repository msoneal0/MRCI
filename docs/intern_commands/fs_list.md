### Summary ###

list all files or sub-directories in a directory.

### IO ###

```[{-path (text)} {-info_frame}]/[text] or [FILE_INFO]```

### Description ###

this list all files in the current directory or the directory specified in -path. this command normally returns human readable text for each file or sub-directory that is listed but you can pass -info_frame to make the command return FILE_INFO frames for each file/sub-directory instead. note: if displaying as text, all directory names are displayed with a '/' at the end.