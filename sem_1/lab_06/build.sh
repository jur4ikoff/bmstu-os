#!/bin/bash

# gcc -o mydaemon main.c -std=c99
gcc -Wall -std=c99 -pthread -o my_daemon main.c