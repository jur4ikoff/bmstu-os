FROM ubuntu:22.04

# Установка зависимостей
RUN apt-get update && apt-get install -y \
    build-essential \
    rpcbind \
    libtirpc-dev \
    libnsl-dev \
    rpcsvc-proto \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

# Компилируем проект (использует Makefile)
RUN make

COPY start.sh /start.sh
RUN chmod +x /start.sh

ENTRYPOINT ["/start.sh"]