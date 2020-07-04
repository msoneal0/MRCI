#!/bin/sh
systemctl -q stop $app_target
systemctl -q disable $app_target
rm -v /etc/systemd/system/$app_target.service
rm -v /usr/bin/$app_target
rm -rv $install_dir
deluser $app_target
echo "Uninstallation Complete"