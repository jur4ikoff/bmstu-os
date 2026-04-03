#!/bin/bash

gcc -std=c99 -pthread -o client client.c
gcc -std=c99 -pthread -o server server.c