$app_target -stop
schtasks /delete /f /tn $app_name
del /f "%windir%\$app_target.exe"
rd /q /s "$install_dir"
echo "Uninstallation Complete"