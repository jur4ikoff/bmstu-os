# Используем официальный образ Ubuntu (можно заменить на alpine, debian и т.д.)
FROM ubuntu:22.04

# Обновляем пакеты и устанавливаем GCC и make (часто нужен вместе с GCC)
RUN apt-get update && \
    apt-get install -y gcc make && \
    apt-get clean && \
    apt-get install -y libtirpc-dev rpcbind && \
    rm -rf /var/lib/apt/lists/*

# Устанавливаем рабочую директорию (опционально)
WORKDIR /app

# По умолчанию — ничего не запускаем, но можно добавить CMD, если нужно
CMD ["/bin/bash"]


# docker build -t my-ubuntu-gcc .
# docker run -it --rm -v "$PWD":/app my-ubuntu-gcc
# rpcbind