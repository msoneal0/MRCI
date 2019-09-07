### Summary ###

create a new host group.

### IO ###

```[-name (text)]/[text]```

### Description ###

this will create a new host group with the unique group name given in -name. all new groups are initialized at rank 2. this can be reconfigured at any time using various group editing commands. host groups are used to manage the access level of all users. each user registered in the host must be assigned a group and each group must be assigned a host rank. the rank is numeric integer that indicates the value of the group's rank. The lower it's numeric value, the higher the level of access the users in that group have in the host with 0 being invalid.