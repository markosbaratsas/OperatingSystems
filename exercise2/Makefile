.PHONY: all clean

all: main1.1 main1.2 main1.3 main1.4

CC = gcc
CFLAGS = -g -Wall -O2
SHELL= /bin/bash

main1.1: main1.1.c tree.o proc-common.o 
	$(CC) $(CFLAGS) $^ -o $@

main1.2: main1.2.c tree.o proc-common.o 
	$(CC) $(CFLAGS) $^ -o $@

main1.3: main1.3.c tree.o proc-common.o 
	$(CC) $(CFLAGS) $^ -o $@

main1.4: main1.4.c tree.o proc-common.o 
	$(CC) $(CFLAGS) $^ -o $@

%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean: 
	rm -f *.o tree-example fork-example pstree-this ask2-{fork,tree,signals,pipes}
