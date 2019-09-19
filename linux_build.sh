#!/bin/sh

qt_dir="$1"
installer_file="$2"

src_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
bin_name="mrci"
app_version="1.0.0"
app_name="MRCI"
install_dir="/opt/$bin_name"
bin_dir="/usr/bin"
tmp_dir="$HOME/.cache/mrci_build"
user="$USER"

if [ ! -d "$qt_dir" ]; then

 echo "a valid path to Qt was not provided, falling back to the default: /usr/lib/x86_64-linux-gnu/qt5/bin"

 qt_dir="/usr/lib/x86_64-linux-gnu/qt5/bin"
 
else

 PATH=$qt_dir:$PATH
 
fi

if [ "$installer_file" = "" ]; then
 
 installer_file="$src_dir/$bin_name-$app_version.run"
  
fi
 
if [ -d "$tmp_dir" ]; then
 
 rm -rfv $tmp_dir
  
fi

if [ $? -eq 0 -a -d "$qt_dir" ]; then

  mkdir -vp $tmp_dir
  cp -rv $src_dir/. $tmp_dir
  cd $tmp_dir
  qmake -config release
 
  if [ $? -eq 0 ]; then
   
   make
   
   if [ $? -eq 0 ]; then
     
    mkdir -v ./build/
    mkdir -v ./build/sqldrivers
    mkdir -v ./build/lib
    ldd ./$bin_name | grep "libQt" | awk '{print $3}' | xargs -I '{}' cp -v '{}' ./build/lib
    ldd ./$bin_name | grep "libicu" | awk '{print $3}' | xargs -I '{}' cp -v '{}' ./build/lib
    ldd ./$bin_name | grep "libssl" | awk '{print $3}' | xargs -I '{}' cp -v '{}' ./build/lib
    ldd ./$bin_name | grep "libcrypto" | awk '{print $3}' | xargs -I '{}' cp -v '{}' ./build/lib
    mv -v ./$bin_name ./build/$bin_name
    cp -v $qt_dir/../plugins/sqldrivers/libqsqlite.so ./build/sqldrivers
    
    startup_script="./build/$bin_name.sh"
    setup_script="./build/setup.sh"
    uninstall_script="./build/uninstall.sh"
    service_file="./build/$bin_name.service"
    
    echo "#!/bin/sh" > $startup_script
    echo "export QTDIR=$install_dir" >> $startup_script
    echo "export QT_PLUGIN_PATH=$install_dir" >> $startup_script
    echo "export LD_LIBRARY_PATH=\"$install_dir/lib:\$LD_LIBRARY_PATH\"" >> $startup_script
    echo "$install_dir/$bin_name \$1 \$2 \$3" >> $startup_script
   
    echo "#!/bin/sh" > $setup_script
    echo "if [ -f \"$install_dir/uninstall.sh\" ]; then" >> $setup_script
    echo " sh $install_dir/uninstall.sh" >> $setup_script
    echo "fi" >> $setup_script
    echo "if [ ! -d \"$install_dir\" ]; then" >> $setup_script
    echo " sudo mkdir -p $install_dir" >> $setup_script
    echo "fi" >> $setup_script
    echo "sudo cp -rfv ./lib $install_dir" >> $setup_script
    echo "sudo cp -rfv ./sqldrivers $install_dir" >> $setup_script
    echo "sudo cp -fv ./$bin_name $install_dir" >> $setup_script
    echo "sudo cp -fv ./$bin_name.sh $install_dir" >> $setup_script
    echo "sudo cp -fv ./uninstall.sh $install_dir" >> $setup_script
    echo "sudo cp -fv ./$bin_name.service /etc/systemd/system/$bin_name@$USER.service" >> $setup_script
    echo "sudo chmod 755 $install_dir/$bin_name" >> $setup_script
    echo "sudo chmod 755 $install_dir/$bin_name.sh" >> $setup_script
    echo "sudo chmod 755 $install_dir/uninstall.sh" >> $setup_script
    echo "sudo chmod 755 $install_dir" >> $setup_script
    echo "sudo chmod -R 755 $install_dir/lib" >> $setup_script
    echo "sudo chmod -R 755 $install_dir/sqldrivers" >> $setup_script
    echo "sudo chmod 755 /etc/systemd/system/$bin_name@$USER.service" >> $setup_script
    echo "sudo ln -sf $install_dir/$bin_name.sh $bin_dir/$bin_name" >> $setup_script
    echo "sudo systemctl start $bin_name@$USER" >> $setup_script
    echo "sudo systemctl enable $bin_name@$USER" >> $setup_script
    echo "echo \"\nInstallation finished. If you ever need to uninstall this application, run this command:\n\"" >> $setup_script
    echo "echo \"   sh $install_dir/uninstall.sh\n\"" >> $setup_script
    
    echo "[Unit]" > $service_file
    echo "Description=$app_name host" >> $service_file
    echo "After=network.target" >> $service_file
    echo "" >> $service_file
    echo "[Service]" >> $service_file
    echo "Type=simple" >> $service_file
    echo "User=%i" >> $service_file
    echo "ExecStart=/usr/bin/env $bin_name -host" >> $service_file
    echo "" >> $service_file
    echo "[Install]" >> $service_file
    echo "WantedBy=multi-user.target" >> $service_file
     
    echo "#!/bin/sh" > $uninstall_script
    echo "sudo systemctl -q stop $bin_name@$USER" >> $uninstall_script
    echo "sudo systemctl -q disable $bin_name@$USER" >> $uninstall_script
    echo "sudo rm -v /etc/systemd/system/$bin_name@$USER.service" >> $uninstall_script
    echo "sudo rm -v $bin_dir/$bin_name" >> $uninstall_script
    echo "sudo rm -rv $install_dir" >> $uninstall_script
    
    chmod +x $setup_script
    
    makeself ./build $installer_file "$app_name Installation" ./setup.sh
    
   fi
    
  fi
   
fi

if [ -d "$tmp_dir" ]; then
 
 rm -rf $tmp_dir
  
fi
