### Summary ###

upload a new module to install into the host.

### IO ###

```[-client_file (path) -name (text) -len (int)]/[text]```

### Description ###

upload a new module to the host using an archive file (.zip, .tar, .7z, etc...) or a loadable library file (.so, .dll, .a, etc...) using the GEN_FILE data type id. the module name is given in -name while GEN_FILE specific arguments like -len determine the size of the file being sent and -client_file determine the file location on the client's file system. note: the host will use -client_file argument to read the file's suffix and determine if it is an archive or library file.

once successfully unloaded; if it is an archive file, all of it's contents are extracted to the module installation directory. the archive file must contain a loadable library file called 'main' with any of the host platform compatible library suffixes like (.so, .dll, .a, etc...). if a library file is uploaded, it is simply copied over to the module installation directory as 'main.'

if successfully installed, all sessions are notified to load the new module. even after a successful install, the module can still fail to load. check the host debug log for failure details if that is the case.