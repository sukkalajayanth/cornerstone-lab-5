CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -O2

all: vm assembler

vm: vm.o vm_main.o
	$(CC) $(CFLAGS) -o vm vm.o vm_main.o

assembler: assembler.o vm.o
	$(CC) $(CFLAGS) -o assembler assembler.o vm.o

vm.o: vm.c vm.h
assembler.o: assembler.c vm.h
vm_main.o: vm_main.c vm.h

clean:
	rm -f *.o vm assembler
