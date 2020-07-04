#!/bin/sh
export QTDIR=$install_dir
export QT_PLUGIN_PATH=$install_dir
export LD_LIBRARY_PATH="$install_dir/lib:\$LD_LIBRARY_PATH"
$install_dir/$app_target $1 $2 $3