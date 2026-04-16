all:
	$(MAKE) -C multithread
	$(MAKE) -C epoll
	$(MAKE) proc_info

proc_info: proc/proc_info.c
	gcc -Wall -Wextra -O2 -o proc/proc_info proc/proc_info.c


build:
	docker build -t lab06 .

run:
	docker run --privileged --rm -v $(PWD):/lab lab06 bash /lab/proc/run.sh

merge:
	printf "m_more_c\tm_more_p\tk_more_c\tk_more_p\n" > results.txt
	paste m_more_c.txt m_more_p.txt k_more_c.txt k_more_p.txt >> results.txt

clean:
	$(MAKE) -C multithread clean
	$(MAKE) -C epoll clean
	rm -f results.txt proc/proc_info proc/proc_multithread.txt proc/proc_epoll.txt proc/proc_*.txt

.PHONY: all build run merge clean
