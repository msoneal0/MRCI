#!/usr/bin/python3

import os
import re
import subprocess
import shutil
import platform
import sys

def get_app_target(text):
    return re.search(r'(APP_TARGET) +(\"(.*?)\")', text).group(3)

def get_app_ver(text):
    return re.search(r'(APP_VER) +(\"(.*?)\")', text).group(3)

def get_app_name(text):
    return re.search(r'(APP_NAME) +(\"(.*?)\")', text).group(3)

def get_qt_path():
    try:
        return str(subprocess.check_output(["qtpaths", "--binaries-dir"]), 'utf-8').strip()
        
    except CalledProcessError:
        print("A call to 'qtpaths' to get the QT installation bin folder failed.")
        
        return raw_input("Please enter the QT bin path (leave blank to cancel the build): ")

def get_qt_from_cli():
    for arg in sys.argv:
        if arg == "-qt_dir":
            index = sys.argv.index(arg)
            
            try:
                return sys.argv[index + 1]
            
            except:
                return ""
            
    return ""

def get_db_header():
    current_dir = os.path.dirname(__file__)
    
    if current_dir == "":
        return "src" + os.sep + "db.h"
    else:
        return current_dir + os.sep + "src" + os.sep + "db.h"
    
def cd():
    current_dir = os.path.dirname(__file__)
    
    if current_dir != "":
        os.chdir(current_dir)
        
def verbose_copy(src, dst):
    print("cpy: " + src + " --> " + dst)
    
    if os.path.isdir(src):
        if os.path.exists(dst) and os.path.isdir(dst):
            shutil.rmtree(dst)
            
        shutil.copytree(src, dst)
    
    else:
        shutil.copyfile(src, dst)
        
def linux_build_app_dir(app_ver, app_name, app_target, qt_bin):
    if not os.path.exists("app_dir/linux/sqldrivers"):
        os.makedirs("app_dir/linux/sqldrivers")
    
    if not os.path.exists("app_dir/linux/lib"):
        os.makedirs("app_dir/linux/lib")
        
    verbose_copy(qt_bin + "/../plugins/sqldrivers/libqsqlite.so", "app_dir/linux/sqldrivers/libqsqlite.so")
    verbose_copy("build/" + app_target, "app_dir/linux/" + app_target)
    
    shutil.copyfile("build/" + app_target, "/tmp/" + app_target)
    # copying the executable file from the build folder to
    # temp bypasses any -noexe retrictions a linux file
    # system may have. there is a chance temp is also
    # restricted in this way but that kind of config is
    # rare. ldd will not run correctly with -noexe 
    # enabled.
    
    lines = str(subprocess.check_output(["ldd", "/tmp/" + app_target]), 'utf-8').split("\n")
    
    os.remove("/tmp/" + app_target)
    
    for line in lines:
        if " => " in line:
            if ("libQt" in line) or ("libicu" in line) or ("libssl" in line) or ("libcrypto" in line):
                if " (0x0" in line:
                    start_index = line.index("> ") + 2
                    end_index = line.index(" (0x0")
                    src_file = line[start_index:end_index]
                    file_name = os.path.basename(src_file)
                    
                    verbose_copy(src_file, "app_dir/linux/lib/" + file_name)
                    
    with open("app_dir/linux/" + app_target + ".sh", "w") as file:
        file.write("#!/bin/sh\n")
        file.write("export QTDIR=$install_dir\n")
        file.write("export QT_PLUGIN_PATH=$install_dir\n")
        file.write("export LD_LIBRARY_PATH=\"$install_dir/lib:\$LD_LIBRARY_PATH\"\n")
        file.write("$install_dir/" + app_target + " $1 $2 $3\n")
        
    with open("app_dir/linux/" + app_target + ".service", "w") as file:
        file.write("[Unit]\n")
        file.write("Description=" + app_name + " Host Daemon\n")
        file.write("After=network.target\n\n")
        file.write("[Service]\n")
        file.write("Type=simple\n")
        file.write("User=" + app_target + "\n")
        file.write("Restart=on-failure\n")
        file.write("RestartSec=5\n")
        file.write("TimeoutStopSec=infinity\n")
        file.write("ExecStart=/usr/bin/env " + app_target + " -host\n")
        file.write("ExecStop=/usr/bin/env " + app_target + " -stop\n\n")
        file.write("[Install]\n")
        file.write("WantedBy=multi-user.target\n")
        
    with open("app_dir/linux/uninstall.sh", "w") as file:
        file.write("#!/bin/sh\n")
        file.write("systemctl -q stop " + app_target + "\n")
        file.write("systemctl -q disable " + app_target + "\n")
        file.write("rm -v /etc/systemd/system/" + app_target + ".service\n")
        file.write("rm -v /usr/bin/" + app_target + "\n")
        file.write("rm -rv $install_dir\n")
        file.write("deluser " + app_target + "\n")
        
    complete(app_ver)

def windows_build_app_dir():
    print("Windows support is work in progress. Check for an update at a later time.")
    # to do: fill out code for windows support here.
    
def complete(app_ver):
    print("Build complete for version: " + app_ver)
    print("You can now run the install.py script to install onto this machine or create an installer.")
    
def main():
    with open(get_db_header()) as file:
        text = file.read()
        
        app_target = get_app_target(text)
        app_ver = get_app_ver(text)
        app_name = get_app_name(text)
        qt_bin = get_qt_from_cli()
        
        if qt_bin is "":
            qt_bin = get_qt_path()
            
        if qt_bin != "":
            print("app_target  = " + app_target)
            print("app_version = " + app_ver)
            print("app_name    = " + app_name)
            print("qt_bin      = " + qt_bin)
            
            cd()
            
            result = subprocess.run([qt_bin + os.sep + "qmake", "-config", "release"])
            
            if result.returncode == 0:
                result = subprocess.run(["make"])
                
            if result.returncode == 0:
                if not os.path.exists("app_dir"):
                    os.makedirs("app_dir")
                    
                with open("app_dir" + os.sep + "info.txt", "w") as info_file:
                    info_file.write(app_target + "\n")
                    info_file.write(app_ver + "\n")
                    info_file.write(app_name + "\n")
                
                if platform.system() == "Linux":
                    linux_build_app_dir(app_ver, app_name, app_target, qt_bin)
                                    
                elif platform.system() == "Windows":
                    windows_build_app_dir()
                                    
                else:
                    print("The platform you are running in is not compatible with the app_dir build out procedure.")
                    print("  output from platform.system() = " + platform.system())    
  
if __name__ == "__main__":
    main()