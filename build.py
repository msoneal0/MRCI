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
        
    except:
        print("A direct call to 'qtpaths' has failed so automatic retrieval of the QT bin folder is not possible.")
        
        return input("Please enter the QT bin path (leave blank to cancel the build): ")

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

def get_nearest_subdir(path, sub_name):
    dir_list = os.listdir(path)
    ret = ""

    for entry in dir_list:
        if sub_name in entry:
            ret = entry

            break

    return ret

def get_maker(qt_path):
    ret = ""

    if platform.system() == "Linux":
        ret = "make"
                                    
    elif platform.system() == "Windows":
        path = os.path.abspath(qt_path + "\\..")
        name = os.path.basename(path)

        if "mingw" in name:
            tools_path = os.path.abspath(qt_path + "\\..\\..\\..\\Tools")
            mingw_ver = name[5:7]
            mingw_tool_subdir = get_nearest_subdir(tools_path, "mingw" + mingw_ver)
            mingw_tool_path = tools_path + "\\" + mingw_tool_subdir + "\\bin"

            if not os.environ['PATH'].endswith(";"):
                os.environ['PATH'] = os.environ['PATH'] + ";"
            
            os.environ['PATH'] = os.environ['PATH'] + mingw_tool_path
            
            ret = "mingw32-make"

        elif "msvc" in name:
            print("Warning: this script will assume you already ran the VsDevCmd.bat or vsvars32.bat script files")
            print("         for Microsoft Visual Studio. Either way, a call to 'nmake' should be recognizable as ")
            print("         a shell command otherwise this script will fail.\n")

            ans = input("If that is the case enter 'y' to continue or any other key to cancel the build: ")

            if ans == 'y' or ans == 'Y':
                ret = "nmake"
            
            else:
                exit()
            
    else:
        print("The system platform is unknown. Output from platform.system() = " + platform.system())

    return ret 
    
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
    verbose_copy(qt_bin + "/../plugins/sqldrivers/libqsqlodbc.so", "app_dir/linux/sqldrivers/libqsqlodbc.so")
    verbose_copy(qt_bin + "/../plugins/sqldrivers/libqsqlpsql.so", "app_dir/linux/sqldrivers/libqsqlpsql.so")
    verbose_copy("build/linux/" + app_target, "app_dir/linux/" + app_target)
    
    shutil.copyfile("build/linux/" + app_target, "/tmp/" + app_target)
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

    verbose_copy("templates/linux_run_script.sh", "app_dir/linux/" + app_target + ".sh")
    verbose_copy("templates/linux_service.service", "app_dir/linux/" + app_target + ".service")
    verbose_copy("templates/linux_uninstall.sh", "app_dir/linux/uninstall.sh")
        
    complete(app_ver, app_target)

def windows_build_app_dir(app_ver, app_name, app_target, qt_bin):
    if os.path.exists("release"):
        os.removedirs("release")

    if os.path.exists("debug"):
        os.removedirs("debug")

    if not os.path.exists("app_dir\\windows"):
        os.makedirs("app_dir\\windows")

    verbose_copy("build\\windows\\" + app_target + ".exe", "app_dir\\windows\\" + app_target + ".exe")
    verbose_copy("templates\\windows_uninstall.bat", "app_dir\\windows\\uninstall.bat")
    verbose_copy("templates\\windows_shtask.xml", "app_dir\\windows\\shtask.xml")
    os.chdir("app_dir\\windows\\")

    result = subprocess.run([qt_bin + "\\" + "windeployqt", app_target + ".exe"])

    cd()

    if result.returncode == 0:
        complete(app_ver, app_target)
    
def complete(app_ver, app_target):
    if os.path.exists("Makefile"):
        os.remove("Makefile")

    if os.path.exists("Makefile.Debug"):
        os.remove("Makefile.Debug")

    if os.path.exists("Makefile.Release"):
        os.remove("Makefile.Release")

    if os.path.exists("object_script." + app_target + ".Debug"):
        os.remove("object_script." + app_target + ".Debug")

    if os.path.exists("object_script." + app_target + ".Release"):
        os.remove("object_script." + app_target + ".Release")

    print("Build complete for version: " + app_ver)
    print("You can now run the install.py script to install onto this machine or create an installer.")
    
def main():
    with open(get_db_header()) as file:
        text = file.read()
        
        app_target = get_app_target(text)
        app_ver = get_app_ver(text)
        app_name = get_app_name(text)
        qt_bin = get_qt_from_cli()
        
        if qt_bin == "":
            qt_bin = get_qt_path()

        maker = get_maker(qt_bin)
            
        if qt_bin != "":
            print("app_target  = " + app_target)
            print("app_version = " + app_ver)
            print("app_name    = " + app_name)
            print("qt_bin      = " + qt_bin)
            print("maker       = " + maker + "\n")

            if maker == "":
                print("Could not find a valid maker/compiler on this platform, unable to continue.")

            else:
                cd()
                
                result = subprocess.run([qt_bin + os.sep + "qmake", "-config", "release"])
            
                if result.returncode == 0:
                    result = subprocess.run([maker])
                
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
                        windows_build_app_dir(app_ver, app_name, app_target, qt_bin)
                    
                    else:
                        print("The platform you are running in is not compatible with the app_dir build out procedure.")
                        print("  output from platform.system() = " + platform.system())

if __name__ == "__main__":
    main()