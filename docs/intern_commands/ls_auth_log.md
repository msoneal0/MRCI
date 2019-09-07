### Summary ###

display or manage the host authorization activity log.

### IO ###

```[{search_terms} {-delete}]/[text]```

### Description ###

by default, all entries in the table are displayed in 50 entries per page. you can pass the column names as -column_name (text) to refine your search to specific entries. this command can handle the following columns:

-time_stamp
-ip_address
-user_name
-auth_attempt
-recover_attempt
-count_to_threshold
-accepted

you can also pass -delete that will cause the command to delete the entries instead of displaying them. note: passing no search terms with this option will delete all entries in the table.

the host use entries in this table to enforce maximum failed login thresholds using a combination of the values found in the -count_to_threshold, -user_name, -count_to_threshold and -accepted columns. so for example, if the host counts a maximum of 50 entries for a certain -user_name with -count_to_threshold = 1 and -accepted = 0 and the host maximum amount of failed attempts is set to 50 then the host will then auto lock the user account to protect it.

the -accepted column is a 1 or 0 to indicate if the user successfully logged in or not and the -count_to_threshold column is also 1 or 0 to determine if this particular entry should be counted toward the threshold or not.