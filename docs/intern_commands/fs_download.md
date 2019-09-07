### Summary ###

download a single file from the host.

### IO ###

```[-remote_file (text) {-client_file (text)} {-len (int)} {-offset (int)} {-single_step} {-force} {-truncate}]/[GEN_FILE]```

### Description ###

this command sends GEN_FILE data of the file given in -remote_file. depending on the client, you might need to enter the destination file in -client_file.

-offset is the position in the file to start reading and it defaults to 0 if not given.
-len is the amount of data to read from the file and the host will auto fill it to the file size if not given. the host also auto fill to the file size if it larger than the actual file size.
-single_step enables GEN_FILE's single step mode if the client/host desires it.
-force bypasses any overwrite confirmation question if the client does such a thing.
-truncate tells the client if it should truncate the destination file. the host does nothing with this. it's entirely up to the client to support it.