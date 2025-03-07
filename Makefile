CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
RM = rm -f
HEADERS = config.h
EXECS = ping traceroute discovery
IP = 8.8.8.8

.PHONY: all default clean runp runsp

all: $(EXECS)

default: all

$(EXECS): %: %.o config.o
	$(CC) $^ -o $@

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

runp: ping
	sudo ./ping -a $(IP) -t 6

runsp: ping
	sudo strace ./ping -a $(IP) -t 6

runt: traceroute
	sudo ./traceroute -a $(IP)

runst: traceroute
	sudo strace ./traceroute -a $(IP)

clean:
	$(RM) *.o *.so $(EXECS)