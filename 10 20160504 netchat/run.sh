#!/bin/sh -e

gnome-terminal -e 'gdb ./server'
sleep 1
gnome-terminal -e ./client
#gnome-terminal -e ./client
