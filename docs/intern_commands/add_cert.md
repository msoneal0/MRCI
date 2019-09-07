### Summary ###

install a new SSL/TLS cert into the host from a local cert and private key file.

### IO ###

```[-name (text) -cert (path) -priv (path) {-force}]/[text]```

### Description ###

add an SSL/TLS certificate to a common name given in -name. the common name is used by clients to request certain certificate installed in the host. certificates usually come in seperate cert file and private key file pairs. -cert is the file path to the cert file and -priv is the path to the private key. the files must be formatted in Pem or Der (extension doesn't matter). once installed, the file pairs no longer need to exists for the host to use them. you can the pass the optional -force option to replace a common name if it already exists without asking a confirmation question.