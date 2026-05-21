# kernel module part1
## Запуск
sudo dmesg -W   Show kernel log

sudo insmod main.ko   load module into kernel
sudo rmmod main.ko    unload module from kernel

## Объяснение кода

## Загадки
### сколько. точек входа и какие
6, open, read, write, release, init, exit

### когда открывается опен и релиз

### когда это происходит в вашем конкретном случае

### почему нужны спциальные функции copy_to_user
ядро находится в физисеской памяти, а процессы имеют только виртуальные адрессные пространства