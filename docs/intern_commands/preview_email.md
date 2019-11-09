### Summary ###

preview the confirmation or password reset emails with dummy values.

### IO ###

```[-reset_email] or [-confirm_email]/[text]```

### Description ###

preview the reset password email with the -reset_email argument or preview the email confirmation with -confirm_email. this prints the subject first and then the message body with the keywords substituted the dummy values/text. this command will output the preview as configured so if the messages do contain html, it will output the html as is. it will be up to the client to parse the html or not.