FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /lab

COPY . .

RUN gcc -Wall -Wextra -O2 -o proc/proc_info proc/proc_info.c
RUN cd multithread && gcc -Wall -Wextra -O2 -o server server.c -lpthread

CMD ["/bin/bash"]
