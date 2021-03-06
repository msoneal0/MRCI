### 7.1 Internal Commands ###

The host is extendable via 3rd party modules but the host itself is an internal module that load commands with direct access to the host database.

* [accept_ch](intern_commands/accept_ch.md) - accept an invite to a channel to become a regular member of it.

* [add_acct](intern_commands/add_acct.md) - create a new host user account.

* [add_ch](intern_commands/add_ch.md) - create a new channel.

* [add_mod](intern_commands/add_mod.md) - add a new module to the host.

* [add_ranked_cmd](intern_commands/add_ranked_cmd.md) - assign a rank to a module's command name.

* [add_rdonly_flag](intern_commands/add_rdonly_flag.md) - add a read only flag to a certain sub-channel and privilege level.

* [add_sub_ch](intern_commands/add_sub_ch.md) - create a new sub-channel within a channel.

* [auth](intern_commands/auth.md) - login to the host using a registered user account name or email address.

* [cast](intern_commands/cast.md) - broadcast data to all sessions listening to any matching sub-channels.

* [ch_owner_override](intern_commands/ch_owner_override.md) - set/unset the channel owner override flag for your current session.

* [close_sub_ch](intern_commands/close_sub_ch.md) - close a sub-channel for your current session.

* [decline_ch](intern_commands/decline_ch.md) - decline an invite to a channel.

* [find_ch](intern_commands/find_ch.md) - search for channels within the host based on the channel name or channel id.

* [force_set_email](intern_commands/force_set_email.md) - overwrite/change the email address of another user's account.

* [fs_cd](intern_commands/fs_cd.md) - display or change the current directory for the current session.

* [fs_copy](intern_commands/fs_copy.md) - copy a file or directory in the host file system.

* [fs_delete](intern_commands/fs_delete.md) - delete a file or directory in the host file system.

* [fs_download](intern_commands/fs_download.md) - download a single file from the host.

* [fs_info](intern_commands/fs_info.md) - get detailed information about a file in the host file system.

* [fs_list](intern_commands/fs_list.md) - list all files or sub-directories in a directory.

* [fs_mkpath](intern_commands/fs_mkpath.md) - attempt to create a directory and all sub-directories of a given path.

* [fs_move](intern_commands/fs_move.md) - move/rename a file or directory in the host file system.

* [fs_tree](intern_commands/fs_tree.md) - list all files and sub-directories of an entire directory tree.

* [fs_upload](intern_commands/fs_upload.md) - upload a single file to the host.

* [host_info](intern_commands/host_info.md) - display system information about the host.

* [invite_to_ch](intern_commands/invite_to_ch.md) - invite a host user to join a channel.

* [is_email_verified](intern_commands/is_email_verified.md) - check if your email address is verified.

* [lock_acct](intern_commands/lock_acct.md) - lock a user account.

* [ls_act_log](intern_commands/ls_act_log.md) - display or manage the client activity log.

* [ls_auth_log](intern_commands/ls_auth_log.md) - display the host authorization activity log.

* [ls_ch_members](intern_commands/ls_ch_members.md) - list all members in a channel.

* [ls_chs](intern_commands/ls_chs.md) - list all channels you are currently a member of and all pending invites.

* [ls_mods](intern_commands/ls_mods.md) - list all available modules currently configured in the host.

* [ls_open_chs](intern_commands/ls_open_chs.md) - list all of the sub-channels that are currently open.

* [ls_p2p](intern_commands/ls_p2p.md) - list all p2p connections and pending p2p request you currently have.

* [ls_ranked_cmds](intern_commands/ls_ranked_cmds.md) - list all module commands with assigned host ranks.

* [ls_rdonly_flags](intern_commands/ls_rdonly_flags.md) - list all read only flags currently present for a channel.

* [ls_sub_chs](intern_commands/ls_sub_chs.md) - list all sub-channels within a channel.

* [ls_users](intern_commands/ls_users.md) - list all users currently registered in the host database.

* [my_info](intern_commands/my_info.md) - display information about your current session and your account.

* [open_sub_ch](intern_commands/open_sub_ch.md) - open a sub-channel to send/receive broadcasted data to/from peers.

* [p2p_close](intern_commands/p2p_close.md) - close the p2p connection with the peer given in this command or decline a p2p request.

* [p2p_open](intern_commands/p2p_open.md) - accept the p2p request you may have received from another peer connected to the host.

* [p2p_request](intern_commands/p2p_request.md) - send out a p2p request to the peer session id given in this command.

* [ping_peers](intern_commands/ping_peers.md) - ping all peer sessions with any matching sub-channels to return information about themselves to you.

* [recover_acct](intern_commands/recover_acct.md) - login to a user account using a temporary password.

* [remove_ch_member](intern_commands/remove_ch_member.md) - remove a user as a member of a channel you currently a member of or cancel an invite.

* [rename_ch](intern_commands/rename_ch.md) - rename a channel.

* [rename_sub_ch](intern_commands/rename_sub_ch.md) - rename a sub-channel within a channel.

* [request_new_pw](intern_commands/request_new_pw.md) - enable/disable a password change request for a user on next login.

* [request_new_user_name](intern_commands/request_new_user_name.md) - enable/disable a user name change request for a user on next login.

* [request_pw_reset](intern_commands/request_pw_reset.md) - request a password reset for a user account.

* [rm_acct](intern_commands/rm_acct.md) - delete a user account from the host database.

* [rm_ch](intern_commands/rm_ch.md) - permanently remove a channel and all of it's sub-shannels from the host.

* [rm_mod](intern_commands/rm_mod.md) - remove a module from the host.

* [rm_ranked_cmd](intern_commands/rm_ranked_cmd.md) - remove a rank from a module command name.

* [rm_rdonly_flag](intern_commands/rm_rdonly_flag.md) - remove a read only flag from a certain sub-channel privilege level combination.

* [rm_sub_ch](intern_commands/rm_sub_ch.md) - remove a sub-channel within a channel.

* [set_active_flag](intern_commands/set_active_flag.md) - set or unset the active update flag of a sub-channel.

* [set_disp_name](intern_commands/set_disp_name.md) - change your account display name.

* [set_email](intern_commands/set_email.md) - set the user account email address.

* [set_member_level](intern_commands/set_member_level.md) - set the user privilege levels of a channel member.

* [set_pw](intern_commands/set_pw.md) - update your account password.

* [set_sub_ch_level](intern_commands/set_sub_ch_level.md) - set the lowest privilege level that members need to be in order to open a certain sub-channel.

* [set_user_name](intern_commands/set_user_name.md) - change your account user name.

* [set_user_rank](intern_commands/set_user_rank.md) - change a user account's host rank.

* [to_peer](intern_commands/to_peer.md) - send/receive any data directly with a client connected to the host.

* [verify_email](intern_commands/verify_email.md) - verify your email address by sending a confirmation code to it.

