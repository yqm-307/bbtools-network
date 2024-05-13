#!/bin/bash

installpath="/usr/local/include"
libpath="/usr/local/lib"

sudo rm -rf $installpath/bbt/network
sudo rm -rf $libpath/libbbt_network.so

echo "删除完毕"