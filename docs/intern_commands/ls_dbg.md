### Summary ###

display debug messages from the host instance and all session instances.

### IO ###

```[{search_terms} {-delete}]/[text]```

### Description ###

by default, all entries in the table are displayed in 50 entries per page. you can pass the column names as -column_name (text) to refine your search to specific entries. this command can handle the following columns:

-time_stamp
-log_entry

you can also pass -delete that will cause the command to delete the entries instead of displaying them. note: passing no search terms with this option will delete all entries in the table.