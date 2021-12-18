#!/bin/bash

make
sudo insmod strdev.ko
#sudo chown <user>:<group> /dev/strdev1 - change owner for rw operations without sudo
