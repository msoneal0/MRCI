#!/bin/python3

import os
import subprocess
import shutil
import platform
import sys
import zipfile
import binascii
import tempfile

def cd():
    current_dir = os.path.dirname(__file__)
    
    if current_dir != "":
        os.chdir(current_dir)
        
def get_default_install_dir(app_target, app_name):
    if platform.system() == "Linux":
        return "/opt/" + app_target
    
    else:
        return "C:\\Program Files\\" + app_name
    
def get_default_installer_path(app_ver, app_name):
    return os.path.expanduser("~") + os.sep + app_name + "-" + app_ver + ".py"
        
def get_install_dir(app_target, app_name):
    path = get_default_install_dir(app_target, app_name)
    
    print("The default install directory is: " + path)
    
    while(True):
        ans = input("Do you want to change it? (y/n): ")
        
        if ans == "y" or ans == "Y":
            path = input("Enter a new install directory (leave blank to go back to the default): ")
            path = os.path.normpath(path)
            break
            
        elif ans == "n" or ans == "N":
            break
            
    if path == "":
        return get_default_install_dir(app_target, app_name)
    
    else:
        return path
    
def get_installer_path(app_ver, app_name):
    path = get_default_installer_path(app_ver, app_name)
    
    print("The built .py installer will placed here: " + path)
    
    while(True):
        ans = input("Do you want to change the path? (y/n): ")
        
        if ans == "y" or ans == "Y":
            path = input("Enter a new path (leave blank to go back to the default): ")
            path = os.path.normpath(path)
            break
            
        elif ans == "n" or ans == "N":
            break
            
    if path == "":
        return get_default_installer_path(app_ver, app_name)
    
    else:
        return path
    
def make_install_dir(path):
    try:
        if not os.path.exists(path):
            os.makedirs(path)
            
    except:
        print("Failed to create the install directory, please make sure you are runnning this script with admin rights.")
        
def replace_text(text, old_text, new_text, offs):
    while(True):
        try:
            index = text.index(old_text, offs)
            text = text[:index] + new_text + text[index + len(old_text):]
        
        except ValueError:
            break
    
    return text
        
def sub_copy_file(src, dst, old_text, new_text, offs):
    print("cpy: " + src + " --> " + dst)
    
    text = ""
    
    with open(src, "r") as rd_file:
        text = rd_file.read()
        text = replace_text(text, old_text, new_text, offs)
        
    with open(dst, "w") as wr_file:
        wr_file.write(text)

def verbose_copy(src, dst):
    print("cpy: " + src + " --> " + dst)
    
    if os.path.isdir(src):
        if os.path.exists(dst) and os.path.isdir(dst):
            shutil.rmtree(dst)
            
        shutil.copytree(src, dst)
    
    else:
        shutil.copyfile(src, dst)
        
def verbose_create_symmlink(src, dst):
    print("lnk: " + src + " --> " + dst)
    
    if os.path.exists(dst):
        os.remove(dst)
    
    os.symlink(src, dst)

def local_install(app_target, app_name):
    if platform.system() == "Linux":
        if not os.path.exists("app_dir/linux"):
            print("An app_dir for the Linux platform could not be found.")
            
        else:
            install_dir = get_install_dir(app_target, app_name)
            
            if os.path.exists(install_dir + "/uninstall.sh"):
                subprocess.run([install_dir + "/uninstall.sh"])
            
            make_install_dir(install_dir)
            
            if not os.path.exists("/var/opt/" + app_target):
                os.makedirs("/var/opt/" + app_target)
            
            sub_copy_file("app_dir/linux/" + app_target + ".sh", install_dir + "/" + app_target + ".sh", "$install_dir", install_dir, 0)
            sub_copy_file("app_dir/linux/uninstall.sh", install_dir + "/uninstall.sh", "$install_dir", install_dir, 0)
            
            verbose_copy("app_dir/linux/" + app_target, install_dir + "/" + app_target)
            verbose_copy("app_dir/linux/lib", install_dir + "/lib")
            verbose_copy("app_dir/linux/sqldrivers", install_dir + "/sqldrivers")
            verbose_copy("app_dir/linux/" + app_target + ".service", "/etc/systemd/system/" + app_target + ".service")
            
            verbose_create_symmlink(install_dir + "/" + app_target + ".sh", "/usr/bin/" + app_target)
            
            subprocess.run(["useradd", "-r", app_target])
            subprocess.run(["chmod", "-R", "755", install_dir])    
            subprocess.run(["chmod", "755", "/etc/systemd/system/" + app_target + ".service"])    
            subprocess.run(["chown", "-R", app_target + ":" + app_target, "/var/opt/" + app_target])    
            subprocess.run(["systemctl", "start", app_target])    
            subprocess.run(["systemctl", "enable", app_target])
            
            print("Installation finished. If you ever need to uninstall this application, run this command with root rights:")
            print("    sh " + install_dir + "/uninstall.sh\n")
            
    elif platform.system() == "Windows":
        print("Windows support is work progress. Check for an update at a later time.")
        # to do: fill ot code for windows support here.
        
    else:
        print("The platform you are running in is not compatible.")
        print("  output from platform.system() = " + platform.system())
    
def dir_tree(path):
    ret = []
    
    if os.path.isdir(path):
        for entry in os.listdir(path):
            full_path = os.path.join(path, entry)
            
            if os.path.isdir(full_path):
                for sub_dir_file in dir_tree(full_path):
                    ret.append(sub_dir_file)
                    
            else:
                ret.append(full_path)
                
    return ret

def to_hex(data):
    return str(binascii.hexlify(data))[2:-1]

def from_hex(text_line):
    return binascii.unhexlify(text_line)
        
def make_install(app_ver, app_name):
    path = get_installer_path(app_ver, app_name)
    
    with zipfile.ZipFile("app_dir.zip", "w", compression=zipfile.ZIP_DEFLATED) as zip_file:
        print("Compressing app_dir --")
        
        for file in dir_tree("app_dir"):
            print("adding file: " + file)
            zip_file.write(file)
            
    sub_copy_file(__file__, path, "main(is_sfx=False)", "main(is_sfx=True)\n\n\n", 7700)
        
    with open(path, "a") as dst_file, open("app_dir.zip", "rb") as src_file:
        print("Packing the compressed app_dir into the sfx script file --")
        
        dst_file.write("# APP_DIR\n")
        
        stat = os.stat("app_dir.zip")
        
        while(True):
            buffer = src_file.read(4000000)
            
            if len(buffer) != 0:
                dst_file.write("# " + to_hex(buffer) + "\n")
                
                print(str(src_file.tell()) + "/" + str(stat.st_size))
            
            if len(buffer) < 4000000:
                break
                
    os.remove("app_dir.zip")
    
    print("Finished.")

def sfx():
    abs_sfx_path = os.path.abspath(__file__)
    mark_found = False
    
    os.chdir(tempfile.gettempdir())
    
    with open(abs_sfx_path) as packed_file, open("app_dir.zip", "wb") as zip_file:
        stat = os.stat(abs_sfx_path)
        
        print("Unpacking the app_dir compressed file from the sfx script.")
        
        while(True):
            line = packed_file.readline()
            
            if not line: 
                break
            
            elif mark_found:
                zip_file.write(from_hex(line[2:-1]))
                
                print(str(packed_file.tell()) + "/" + str(stat.st_size))
            
            else:
                if line == "# APP_DIR\n":
                    mark_found = True
                    
        print("Done.")
    
    if not mark_found:
        print("The app_dir mark was not found, unable to continue.")
        
    else:
        with zipfile.ZipFile("app_dir.zip", "r", compression=zipfile.ZIP_DEFLATED) as zip_file:
            print("De-compressing app_dir --")
            
            zip_file.extractall()
            
        print("Preparing for installation.")
        
        os.remove("app_dir.zip")
        
        with open("app_dir" + os.sep + "info.txt") as info_file:
            list = info_file.read().split("\n")
            
            local_install(list[0], list[2])
            shutil.rmtree("app_dir") 

def main(is_sfx):
    cd()
    
    app_target = ""
    app_ver = ""
    app_name = ""
    
    if not is_sfx:
        with open("app_dir" + os.sep + "info.txt") as info_file:
            list = info_file.read().split("\n")
            
            app_target = list[0]
            app_ver = list[1]
            app_name = list[2]

    if is_sfx:
        sfx()
    
    elif "-local" in sys.argv:
        local_install(app_target, app_name)
        
    elif "-installer" in sys.argv:
        make_install(app_ver, app_name)
        
    else:
        print("Do you want to install onto this machine or create an installer?")
        print("[1] local machine")
        print("[2] create installer")
        print("[3] exit")
        
        while(True):
            opt = input("select an option: ")
            
            if opt == "1":
                local_install(app_target, app_name)
                break
                
            elif opt == "2":
                make_install(app_ver, app_name)
                break
                
            elif opt == "3":
                break

if __name__ == "__main__":
    main(is_sfx=False)