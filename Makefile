CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
RM = rm -f
HEADERS = config.h
EXECS = ping traceroute
IP = 1.1.1.1

.PHONY: all default clean runp runsp

all: $(EXECS)

default: all

$(EXECS): %: %.o config.o
	$(CC) $^ -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

runp: ping
	sudo ./ping -a $(IP)

runsp: ping
	sudo strace ./ping -a $(IP)

runt: traceroute
	sudo ./traceroute -a $(IP)

runsp: ping
	sudo strace ./traceroute -a $(IP)

clean:
	$(RM) *.o *.so $(EXECS)