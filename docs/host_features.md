### 4.1 Host Features ###

Other than transporting data to be processed by modules, the host have a few other built in features such as data broadcasting to/from clients connected to the host and full user account management. The following concepts needed to be created to facilitate these specific features:

### 4.2 Host Ranks ###

Each user registered in the host must be assigned a host rank. The lower it's numeric value, the higher the level of access the user has in the host with 0 being invalid. The host defines a default initial host rank for all new users in the host config file located at /etc/mrci/conf.json if running on a linux based os or %PROGRAMDATA%\mrci\conf.json if running on windows.

Some internal commands have the ability to change the user account information of other accounts. The host will in general not allow users of a lower level of access to any user information of higher access level. For example: a user of host rank ```1``` can force change another user's email address if that user's rank is ```2``` or higher but the rank ```2``` user can't do the same in reverse.

Host ranks can also be assigned to the commands themselves via the command names of specific modules. By doing this, the host can be configured to allow users of certain ranks or higher access to running certain commands. For example: if a command named ```this_cmd``` is assigned a host rank of ```6```, all users with a host rank value of ```6``` or lower will have access to running this command. All commands that don't have an assigned rank will be assumed a rank of ```1``` but all commands that define itself as rank exempt can bypass this and allow the user to run it regardless of the user's host rank. Exemptions would also disregard the assigned rank of the command.

### 4.3 Channels And Sub-channels ###

Channels are used to manage sub-channels and sub-channels are used to determine which clients can send/receive broadcast data. When a client broadcast data to a open sub-channel, the clients that want to receive that data will also need to have the matching sub-channel open. Channels and sub-channels can be identified by a string name or integer id. 

When a channel is created, the user only needs to provide a unique channel name because the host will auto generate a unique channel id attached to that name. This id is a 64bit unsigned integer and it remains constant for as long as the channel exists, even when the name changes. The channel however can't broadcast any data until a sub-channel is created with the user providing a sub-channel name unique to the channel and the host will auto generate a sub-channel id also unique to the channel. The sub-id is a 8bit unsigned integer and it also remains constant for as long as the sub-channel exists. 

When brocasting data, a combination of the channel and sub-channel ids are used as described in [ASYNC_CAST](async.md) or [ASYNC_LIMITED_CAST](async.md).

Access to opening sub-channels are managed in a way similar to the host ranks. All channels have a list of host user accounts that are members of the channel, each having managed levels of access to what they can do as a member of the channel. Host users are added via invitation and the users themselves have the option to accept or decline the invitation to join the channel. Like the host rank, access levels to the channels are a numeric integer value where the lower it's value is the higher level of access the channel member will have but unlike the host rank, the access levels are not user defined. Instead, all levels are host defined and what each level can do is also host defined.

```
enum ChannelMemberLevel
{
    OWNER   = 1,
    ADMIN   = 2,
    OFFICER = 3,
    REGULAR = 4,
    PUBLIC  = 5
};
```

Below is a description of what each of these levels can do within the channel:

owner-level(1):

1. delete or rename the channel.
2. delete, create or rename sub-channels within the channel.
3. invite new users to the channel or cancel invites.
4. remove any member of the channel except your self.
5. set sub-channels' level of access.
6. add/remove read only flags to/from sub-channels.
7. update the privilege level of any member in the channel.
8. open/close sub-channels.

admin-level(2):

1. delete, create or rename sub-channels within the channel.
2. invite new users to the channel or cancel invites.
3. remove any member of channel except the owner and other admins.
4. set sub-channels' level of access.
5. add/remove read only flags to/from sub-channels.
6. update the privilege level of members up to your own level but not above.
7. open/close sub-channels.

officer-level(3):

1. invite new users to the channel or cancel invites.
2. can only remove regular members of the channel.
3. update the privilege level of members up to your own level but not above.
4. open/close sub-channels.

regular-level(4):

1. open/close sub-channels.

public-level(5):

1. open/close public sub-channels.

All sub-channels can be configured with it's own "lowest level of access" level that can make it so only certain channel members can open it. For example, the channel owner or admin can set a sub-channel's minimum level to 4 to make it so only channel regular members and above can open the sub-channel or it can be set to 5 to make it so anybody can open/close the sub-channel, affectively making it a public sub-channel.

There can only be one channel owner so if the owner decides to change the privilege of any other member in the channel to owner, the current owner will automatically step down to level 2 (admin). Also note that privilege level 5 is reserved for users that are not a member of the channel and cannot be assigned to any user that are currently members of the channel.

Sub-channels can also be assigned what is called *read-only* flags. These flags are attached to the sub-channel id and privilege level. It is decoupled from all changes that could occur with the sub-channel so this means the sub-channel can get renamed or even deleted but the read-only flag would still remain. What a read-only flag actual does is make it so certain users of the matching level can listen for broadcast data but cannot send out broadcast data to the sub-channel. So a read-only flag for example can be added to a sub-channel id for privilege 5 to make it so public users can listen to the sub-channel but cannot send out anything to it.

### 4.4 Host Config File ###

This application stores important global settings in a single json formatted file located at /etc/mrci/conf.json if running on a Linux based OS or %Programdata%\mrci\conf.json if running on windows. Here is a description of all the settings that are stored in that file and what are considered valid vaules.

```
all_channels_active_update : bool

  This option tells the host if all sub-channels should be considered
  active or not. otherwise, the active flag can be toggled on/off at the
  individual sub-channels. active sub-channels send/receive PEER_INFO or
  PEER_STAT frames to all peer clients connected to the sub-channel so 
  they can be made aware of each other's status and public information.
  without the active flag, no such frames are transferred.

auto_lock_limit : int

  The autolock threshold is an integer value that determines how many
  failed login attempts can be made before the user account is locked
  by the host.

db_driver : string

  The host can support different types of SQL databases depending on 
  what QT database drivers are installed in the system. these drivers 
  are usually found in /opt/mrci/sqldrivers if running on a Linux 
  based OS or %programfiles%\MRCI\sqldrivers if running on Windows.
  you can also run mrci -ls_sql_drvs to get a list of available
  drivers. the default is QSQLITE.

db_host_name : string

  This value contains the network, internet or file location address
  of the database.

db_password : string

  This value contains the authentication password to the database if
  password protected.

db_user_name : string

  This value contains the authentication username to the database if
  password protected.

email_verify_subject : string

  The host will use this string as the email subject when sending an
  email verification email. an email verification basically sends
  a numeric code to the target user's email address and awaits input
  of the code by the user. if the entered code is correct, that
  verifies that the user is the owner of the email address.

email_verify_template : string

  Path to a any text file formatted in UTF8 unicode in the local host
  file system. the text contained in this file will be used in the
  actual email body when sending a email verification email to the
  target user's email address. the message body must contain all of 
  the keywords in section 4.5.

enable_email_verify : bool

  This enables/disables automated email confirmations. this tells the 
  host if it is allowed to load the verify_email command for any user, 
  regardless of rank. 

enable_public_reg : bool

  Public registration basically allows un-logged in clients to run the
  add_acct command. doing this allows un-registered users to become
  registered users without the need to contact an admin.

enable_pw_reset : bool

  This enables automated password resets via email so users can reset 
  their account passwords without the need to contact an admin. this 
  basically tells the host if it is allowed to load the 
  request_pw_reset and recover_acct commands or not.

initial_rank : int

  The initial host rank is the rank all new user accounts are 
  registered with when created. the host rank itself is an integer 
  value that determine what commands each user can or cannot run.
  see section 4.2 for more info on host ranks.

listening_addr : string

  This is the local address that the host listen on for clients.
  the default is 0.0.0.0 which means the host will listen on any
  local address connected to the host.

listening_port : int

  This is the port that the host will listen on for clients.

mail_client_cmd : string

  This is the command line template that the host will use when 
  sending emails. just like the email templates, this command 
  also require certain keywords to be present to successfully
  construct a working command line. see section 4.6 for these
  keywords.

max_sessions : int

  Max sessions is an integar value that determines how many 
  simultaneous clients the host will be allowed to run at once.  

max_sub_channels : int

  This option sets the maximum amount of sub-channels each channel 
  can have. this must range between 1 and 255.

reset_pw_mail_subject : string

  The host will use this string as the email subject when sending a
  password reset email. the email body will contain a temp password
  that the user will need to enter when running the recover_acct
  command and in turn be able to change the account password. 

reset_pw_mail_template : string

  Path to a any text file formatted in UTF8 unicode in the local host
  file system. the text contained in this file will be used in the
  actual email body when sending a password reset email to the target 
  user's email address. the message body must contain all of the 
  keywords in section 4.5.

tls_cert_chain : string

  Path to the SSL/TLS cert file used for secure TCP connections. more 
  than one cert file can be defined to form a complete chain by colon 
  seperating multiple file paths.

tls_priv_key : string

  Path to the SSL/TLS private key used for secure TCP connections.

```

### 4.5 Email Template Keywords ###

The host will replace the following keywords in the email template with actual values/text when contructing the email.

```
 %user_name%  -  the registered username of the target.
 %date%       -  the date and time stamp of the email being sent.
 %otp%        -  standing for "one time password" this keyword is
                 the temporary password or verification code that 
                 is sent to the target's email.
```

### 4.6 Email Command Line Template Keywords ###

The host will replace the following keywords in the command line template with actual values/text before running it.

```
%message_body%  -  the fully built message body text from an email
                   template.
%subject%       -  the email subject text.
%target_email%  -  target email address that the email will be
                   sent to.
```