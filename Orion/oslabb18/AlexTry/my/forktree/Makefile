.PHONY: all clean

all: fork-example tree-example ask2-fork ask2-signals ask2_1 ask2_2 ask2_3 ask2_4

CC = gcc
CFLAGS = -g -Wall -O2
SHELL= /bin/bash

tree-example: tree-example.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

fork-example: fork-example.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-fork: ask2-fork.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

ask2_1: ask2_1.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

ask2-signals: ask2-signals.o proc-common.o tree.o
	$(CC) $(CFLAGS) $^ -o $@

ask2_2: ask2_2.o tree.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@
ask2_3: ask2_3.o tree.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@
ask2_4: ask2_4.o tree.o proc-common.o
	$(CC) $(CFLAGS) $^ -o $@

%.s: %.c
	$(CC) $(CFLAGS) -S -fverbose-asm $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

%.i: %.c
	gcc -Wall -E $< | indent -kr > $@

clean: 
	rm -f *.o tree-example fork-example pstree-this ask2-{fork,tree,signals,pipes}
	rm -f *.o ask2_{1,2,3,4}
	
