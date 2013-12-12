#!/bin/bash
DIR=/opt/433ctrl
$DIR/433srv >/var/log/433srv.log 2>&1 &
echo $! > /var/run/433srv.pid

