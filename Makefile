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
HEADERS = ping.h

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

%: %.c
	$(CC) $(CFLAGS) -c $< -o $@

runp: $(EXECS)
	sudo ./$< $(IP)

runsp: $(EXECS)
	sudo strace ./$< $(IP)

clean:
	$(RM) *.o *.so $(EXECS)