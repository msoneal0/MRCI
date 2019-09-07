### Summary ###

display or manage the host ip address ban table.

### IO ###

```[{search_terms} {-delete}]/[text]```

### Description ###

by default, all entries in the table are displayed in 50 entries per page. you can pass the column names as -column_name (text) to refine your search to specific entries. this command can handle the following columns:

-time_stamp
-ip_address

you can also pass -delete that will cause the command to delete the entries instead of displaying them. note: passing no search terms with this option will delete all entries in the table. keep in mind, the host use this table to enforce the ban-by-ip option. deleting entries in this table will un-ban the ip's affected by the deletion.