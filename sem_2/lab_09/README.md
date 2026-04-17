# kernel module part1
## Запуск
sudo dmesg -W   Show kernel log

sudo insmod main.ko   load module into kernel
sudo rmmod main.ko    unload module from kernel

## Объяснение кода

## Загадки
### 
Берет рандомный процесс и смотрит на `state` и `flags`, нужно расшифровать. 
- [state docs](https://elixir.bootlin.com/linux/v7.0/source/include/linux/sched.h#L107)
- [flags docs](https://elixir.bootlin.com/linux/v7.0/source/include/linux/sched.h#L1752)


comm=systemd, pid=1, ppid_comm=swapper/0, ppid=0, tgid=1, state=0x1, flags=0x400100(10000000000000100000000), on_cpu=0, prio=120, policy=0, exit_state=0, utime=1020000000, stime=886000000, last_switch_count=0, last_switch_time=0, sessionid=4294967295
