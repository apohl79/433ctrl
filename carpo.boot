#!/bin/bash
HOST=192.168.1.100
while [ -z "$(ping -c 1 $HOST|grep 'bytes from')" ]; do 
    sleep 1
done
