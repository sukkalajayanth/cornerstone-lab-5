#include "vm.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <bytecode.bin>\n", argv[0]);
        return 1;
    }

    VM vm;
    vm_init(&vm);
    if (!vm_load_program(&vm, argv[1])) {
        return 1;
    }

    vm_run(&vm);
    vm_dump_stack(&vm);
    return 0;
}
