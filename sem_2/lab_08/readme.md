# Как

```bash
sudo nano /etc/exports
cat /etc/exports 
# /Users/ypopov2005/Programming/bmstu-os/bmstu-os/sem_2/lab_08/shared_dir_server -alldirs -maproot=root -network 127.0.0.0 -mask 255.0.0.0
```

/etc/exports -- конфиг, где содержатся все шары

```bash
sudo nfsd restart
sudo showmount -e          
# Exports list on localhost:
# /Users/ypopov2005/Programming/bmstu-os/bmstu-os/sem_2/lab_08/shared_dir_server loopback
```

```bash
sudo mount -o resvport -t nfs 127.0.0.1:/Users/ypopov2005/Programming/bmstu-os/bmstu-os/sem_2/lab_08/shared_dir_server /Users/ypopov2005/Programming/bmstu-os/bmstu-os/sem_2/lab_08/shared_dir_client
```

## Как запустить с другом

### Найти подсеть

```bash
ifconfig
# en0: flags=8863<UP,BROADCAST,SMART,RUNNING,SIMPLEX,MULTICAST> mtu 1500
#         options=6460<TSO4,TSO6,CHANNEL_IO,PARTIAL_CSUM,ZEROINVERT_CSUM>
#         ether b2:71:e1:38:cd:07
#         inet6 fe80::1053:3322:29f5:b588%en0 prefixlen 64 secured scopeid 0xb 
#         inet 192.168.50.50 netmask 0xffffff00 broadcast 192.168.50.255
#         nd6 options=201<PERFORMNUD,DAD>
#         media: autoselect
#         status: active

# inet -- мой айпишник в сети, чтобы узнать, нужно последний октет напистаь вместо него 0 -- 192.168.50.0 Масска как правило 255.255.255.0
```
В `/etc/exports` записать измененный айпишник

