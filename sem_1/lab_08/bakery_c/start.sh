#!/bin/bash
mkdir -p /run/rpcbind
echo "Starting rpcbind..."
rpcbind
sleep 2
exec "$@"