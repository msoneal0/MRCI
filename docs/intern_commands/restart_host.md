### Summary ###

re-start the host instance.

### IO ###

```[none]/[text]```

### Description ###

this will restart the host instance, along with a database reload. be very careful with this command. it ends all current sessions and since it also reloads the database, host behaviour might also change depending on what is in the new database if the path was changed.