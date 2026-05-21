CC := gcc
CFLAGS := -Wall -Wextra -std=c11
LDLIBS := -pthread

SOURCES := $(wildcard *.c)
PROGRAMS := $(SOURCES:.c=)

.PHONY: all clean

all: $(PROGRAMS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	$(RM) $(PROGRAMS)
