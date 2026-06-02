#!/usr/bin/env bash
make

sudo insmod my_vfs.ko

sudo mkdir -p /mnt/dir1

sudo mkdir -p /mnt/dir2

sudo mount -t myfs none /mnt/dir1

sudo mount -t myfs none /mnt/dir2

sudo umount /mnt/dir1

sudo umount /mnt/dir2

sudo rmmod my_vfs

sudo dmesg | tail -23
