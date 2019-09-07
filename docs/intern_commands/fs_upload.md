### Summary ###

upload a single file to the host.

### IO ###

```[-remote_file (text) -len (int) {-client_file (text)} {-offset (int)} {-single_step} {-force} {-truncate}]/[GEN_FILE]```

### Description ###

attempt to upload the file given in -client_file to the destination file path in the host given in -remote_file. depending on the client, you might not need to enter -len. if supported, the client will auto fill -len with what ever value is needed.

-offset is the position in the file to start writing and it defaults to 0 if not given.
-single_step enables GEN_FILE's single step mode if the client/host desires it.
-force bypasses the overwrite confirmation question if the destination file already exists.
-truncate tells the host if it should truncate the destination file.