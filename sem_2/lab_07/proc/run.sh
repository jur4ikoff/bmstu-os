#!/bin/bash
set -xe

gcc -Wall -Wextra -O2 -o ./proc/proc_info ./proc/proc_info.c
gcc -Wall -Wextra -O2 -o ./dir/chdir ./dir/chdir.c 2>/dev/null
gcc -Wall -Wextra -O2 -o ./multithread/server ./multithread/server.c -lpthread 2>/dev/null
gcc -Wall -Wextra -O2 -o ./multithread/client ./multithread/client.c 2>/dev/null
gcc -Wall -Wextra -O2 -D_GNU_SOURCE -o ./epoll/server ./epoll/server.c -lpthread 2>/dev/null
gcc -Wall -Wextra -O2 -o ./epoll/client ./epoll/client.c 2>/dev/null

(cd ./multithread && exec ./server) &
PID_MT=$!
sleep 2
./multithread/client >/dev/null &
./multithread/client >/dev/null &
./multithread/client >/dev/null &
./multithread/client >/dev/null &
sleep 3
(cd ./proc && ./proc_info $PID_MT "многопоточный сервер" proc_multithread.txt)
kill $PID_MT 2>/dev/null || true
wait
sleep 1

(cd ./epoll && exec ./server) &
PID_EP=$!
sleep 2
./epoll/client >/dev/null &
./epoll/client >/dev/null &
./epoll/client >/dev/null &
./epoll/client >/dev/null &
sleep 3
(cd ./proc && ./proc_info $PID_EP "epoll сервер" proc_epoll.txt)
kill $PID_EP 2>/dev/null || true
wait

(while true; do ./dir/chdir / >/dev/null 2>/dev/null; done) &
PID_LOOP=$!
sleep 1
PID_DIR=$(pgrep -P $PID_LOOP chdir | head -1)
if [ -n "$PID_DIR" ]; then
    (cd ./proc && ./proc_info $PID_DIR "chdir" proc_chdir.txt)
else
    (cd ./proc && ./proc_info $PID_LOOP "chdir" proc_chdir.txt)
fi
kill $PID_LOOP 2>/dev/null || true
wait

ls -lh ./proc/proc_*.txt
