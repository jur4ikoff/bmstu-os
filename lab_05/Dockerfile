FROM --platform=linux/amd64 ubuntu:22.04

# Установка Wine64 и MinGW
RUN apt-get update && apt-get install -y \
    wine \
    gcc-mingw-w64 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

ENV WINEARCH=win64
ENV WINEPREFIX=/root/.wine

CMD ["bash", "-c", "x86_64-w64-mingw32-gcc prog.c -o prog.exe -lkernel32 && wine64 prog.exe"]