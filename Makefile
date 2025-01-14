#######################################
# 	Makefile for the ping program.    #
#	Made by: Roy Simanovich.          #
#	Last modified: 13/02/2024.        #
#	For: Computer Networks course.    #
#	Free to use and distribute.       #
#######################################

# Use the gcc compiler.
CC = gcc

# Flags for the compiler. Can also use -g for debugging.
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic

# Command to remove files.
RM = rm -f

# Header files.
HEADERS = config.h

# Executable files.
EXECS = ping traceroute

# IP address to ping.
IP = 1.1.1.1

# Phony targets - targets that are not files but commands to be executed by make.
.PHONY: all default clean runp runsp

# Default target - compile everything and create the executables and libraries.
all: $(EXECS)

# Alias for the default target.
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