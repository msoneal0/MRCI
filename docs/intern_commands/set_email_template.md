### Summary ###

set the email template used by the host to send emails for user account resets and confirmations.

### IO ###

```[[-reset_template] or [-confirm_template] {-subject {(text)}} {[-body {(text)}] or [-client_file (text) -len (int)]}]/[text]```

### Description ###

this updates the email template used by the host to send emails to the users that request password resets and or email confirmations. the presents of -reset_template updates the password reset email or the presents of -confirm_template updates the confirmation email. 

-subject is exactly as the name implies, it tells the host what subject to use when sending the email.

-body sets the email message body directly from the command line or -client_file loads the body from a text file. if uploading a text file, use the -len parameter to enter the file size in bytes if your client does not auto fill that. 

the message body must contain the following keywords to be acceptable: 

 %user_name%
 %date%
 %temp_pw% (if -reset_template)
 %confirmation_code% (if -confirm_template)

the host will substitute these keywords with actual values/text when contructing the email. note: if sending a text file, the host assumes it is encoded in UTF-16LE. pass an empty -subject and/or a empty -body to reset the values to host defaults.