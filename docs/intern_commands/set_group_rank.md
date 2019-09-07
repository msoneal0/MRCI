### Summary ###

set the host rank of a group.

### IO ###

```[-name (text) -rank (int)]/[text]```

### Description ###

set the host rank for the group name given in -name to the rank given in -rank. the host rank is used throughout this application to determine how much access to the host commands each user attached to the group has. the lower the numeric value of the host rank, the higher the level of access to the host the group has (1 being the highest level of access). you cannot change the group of a group that has a higher rank than your own group.