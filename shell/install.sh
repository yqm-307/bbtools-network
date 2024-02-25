#!/bin/bash

installpath="/usr/local/include"
libpath="/usr/local/lib"

cd ..
shell/bbt_copy_header_dir.sh $installpath bbt network

sudo cp build/lib/libbbt_network.so /usr/local/lib/

if [ ! -d "build" ];then
    mkdir build
fi

