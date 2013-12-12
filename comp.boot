#!/bin/bash
HOST=$1
while [ -z "$(ping -c 1 $HOST|grep 'bytes from')" ]; do 
    sleep 1
done
