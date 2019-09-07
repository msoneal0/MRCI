### Summary ###

delete a user account from the host database.

### IO ###

```[-name (text) {-force}]/[text]```

### Description ###

delete the user account given in -name. this will also automatically kill all sessions currently using this user account. be very careful with this command since it's changes cannot be undone. you can use the -force option to bypass the confirmation question.