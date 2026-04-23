# kernel module part1
## Запуск
sudo dmesg -W   Show kernel log

sudo insmod main.ko   load module into kernel
sudo rmmod main.ko    unload module from kernel

## Объяснение кода

## Загадки
### Определить информацию о рандомном процессе по state и flags
Определить информацию о рандомном процессе по `state` и `flags`, нужно расшифровать. 
- [state docs](https://elixir.bootlin.com/linux/v7.0/source/include/linux/sched.h#L107)
- [flags docs](https://elixir.bootlin.com/linux/v7.0/source/include/linux/sched.h#L1752)

Разобрать migration/0, kworker/0, insmod/0

[ 7237.475557] comm=migration/0, pid=18, parent comm=kthreadd, ppid=2, tgid=18, state=0x1, flags=0x4208040, on_cpu=0, prio=0, policy=1, exit_state=0, utime=0, stime=20000000, last_switch_count=0, last_switch_time=0, sessionid=4294967295

[ 7237.484953] comm=kworker/0:0, pid=8210, parent comm=kthreadd, ppid=2, tgid=8210, state=0x402, flags=0x4208060, on_cpu=0, prio=120, policy=0, exit_state=0, utime=0, stime=645000000, last_switch_count=0, last_switch_time=0, sessionid=4294967295

[ 7237.485734] comm=insmod, pid=10312, parent comm=sudo, ppid=10311, tgid=10312, state=0x0, flags=0x400100, on_cpu=1, prio=120, policy=0, exit_state=0, utime=1000000, stime=14000000, last_switch_count=0, last_switch_time=0, sessionid=3
