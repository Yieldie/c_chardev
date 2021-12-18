# simple_chardev

A driver for a very simple *character device* called "strdev" written as a loadable kernel module for Linux. The device stores a message (initially "none"), that can be read by reading from the device (e.g. ```cat /dev/strdev1```) or overwritten by writing to the device (e.g. ```echo -n "test" > /dev/strdev1```). When reading from the device, besides the message itself, we also receive an information about number of read/write operations performed so far.

Package linux-headers (Arch Linux: ```sudo pacman -S linux-headers```) with kernel headers is required to compile the module. File ```strdev.c``` contains the implementation of the module, in ```scripts``` directory there are two helper scripts: ```insert.sh``` which compiles the module and ```insmod```s it and ```remove.sh``` which ```rmmod```s the module and cleans up the working directory. A device ```/dev/strdev1``` equipped with this driver is automatically created on ```insmod``` and deleted on ```rmmod```. I recommend to do ```sudo chown <user>:<group> /dev/strdev1``` and/or add this line to the insert.sh script, so you can perform rw operations on the device without superuser privileges.
