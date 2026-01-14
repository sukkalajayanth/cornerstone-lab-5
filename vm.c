#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int32_t read_i32_le(const uint8_t *buf, uint32_t pos) {
    return (int32_t)(
        (buf[pos]) |
        (buf[pos + 1] << 8) |
        (buf[pos + 2] << 16) |
        (buf[pos + 3] << 24)
    );
}

void vm_init(VM *vm) {
    vm->sp = -1;
    vm->rsp = -1;
    memset(vm->memory, 0, sizeof(vm->memory));
    vm->program_size = 0;
    vm->pc = 0;
    vm->running = false;
}

bool vm_load_program(VM *vm, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return false;
    }
    size_t n = fread(vm->program, 1, PROGRAM_MAX, f);
    if (ferror(f)) {
        perror("fread");
        fclose(f);
        return false;
    }
    fclose(f);
    vm->program_size = n;
    vm->pc = 0;
    return true;
}

static void push(VM *vm, int32_t v) {
    if (vm->sp >= STACK_MAX - 1) {
        fprintf(stderr, "Stack overflow\n");
        vm->running = false;
        return;
    }
    vm->stack[++vm->sp] = v;
}

static int32_t pop(VM *vm) {
    if (vm->sp < 0) {
        fprintf(stderr, "Stack underflow\n");
        vm->running = false;
        return 0;
    }
    return vm->stack[vm->sp--];
}

static void ret_push(VM *vm, uint32_t addr) {
    if (vm->rsp >= RET_STACK_MAX - 1) {
        fprintf(stderr, "Return stack overflow\n");
        vm->running = false;
        return;
    }
    vm->ret_stack[++vm->rsp] = (int32_t)addr;
}

static uint32_t ret_pop(VM *vm) {
    if (vm->rsp < 0) {
        fprintf(stderr, "Return stack underflow\n");
        vm->running = false;
        return 0;
    }
    return (uint32_t)vm->ret_stack[vm->rsp--];
}

void vm_dump_stack(const VM *vm) {
    printf("Stack (top -> bottom): ");
    for (int i = vm->sp; i >= 0; --i) {
        printf("%d ", vm->stack[i]);
    }
    printf("\n");
}

void vm_run(VM *vm) {
    vm->running = true;

    while (vm->running && vm->pc < vm->program_size) {
        uint8_t opcode = vm->program[vm->pc++];

        switch (opcode) {
            case OP_PUSH: {
                if (vm->pc + 4 > vm->program_size) {
                    fprintf(stderr, "PUSH out of bounds\n");
                    vm->running = false;
                    break;
                }
                int32_t val = read_i32_le(vm->program, vm->pc);
                vm->pc += 4;
                push(vm, val);
                break;
            }
            case OP_POP: {
                (void)pop(vm);
                break;
            }
            case OP_DUP: {
                if (vm->sp < 0) {
                    fprintf(stderr, "DUP on empty stack\n");
                    vm->running = false;
                    break;
                }
                int32_t top = vm->stack[vm->sp];
                push(vm, top);
                break;
            }

            case OP_ADD: {
                int32_t b = pop(vm);
                int32_t a = pop(vm);
                push(vm, a + b);
                break;
            }
            case OP_SUB: {
                int32_t b = pop(vm);
                int32_t a = pop(vm);
                push(vm, a - b);
                break;
            }
            case OP_MUL: {
                int32_t b = pop(vm);
                int32_t a = pop(vm);
                push(vm, a * b);
                break;
            }
            case OP_DIV: {
                int32_t b = pop(vm);
                int32_t a = pop(vm);
                if (b == 0) {
                    fprintf(stderr, "Division by zero\n");
                    vm->running = false;
                    break;
                }
                push(vm, a / b);
                break;
            }
            case OP_CMP: {
                int32_t b = pop(vm);
                int32_t a = pop(vm);
                push(vm, (a < b) ? 1 : 0);
                break;
            }

            case OP_JMP: {
                if (vm->pc + 4 > vm->program_size) {
                    fprintf(stderr, "JMP out of bounds\n");
                    vm->running = false;
                    break;
                }
                uint32_t addr = (uint32_t)read_i32_le(vm->program, vm->pc);
                vm->pc = addr;
                break;
            }
            case OP_JZ: {
                if (vm->pc + 4 > vm->program_size) {
                    fprintf(stderr, "JZ out of bounds\n");
                    vm->running = false;
                    break;
                }
                uint32_t addr = (uint32_t)read_i32_le(vm->program, vm->pc);
                vm->pc += 4;
                int32_t cond = pop(vm);
                if (cond == 0) vm->pc = addr;
                break;
            }
            case OP_JNZ: {
                if (vm->pc + 4 > vm->program_size) {
                    fprintf(stderr, "JNZ out of bounds\n");
                    vm->running = false;
                    break;
                }
                uint32_t addr = (uint32_t)read_i32_le(vm->program, vm->pc);
                vm->pc += 4;
                int32_t cond = pop(vm);
                if (cond != 0) vm->pc = addr;
                break;
            }

            case OP_STORE: {
                if (vm->pc + 4 > vm->program_size) {
                    fprintf(stderr, "STORE out of bounds\n");
                    vm->running = false;
                    break;
                }
                int32_t idx = read_i32_le(vm->program, vm->pc);
                vm->pc += 4;
                int32_t val = pop(vm);
                if (idx < 0 || idx >= MEM_SIZE) {
                    fprintf(stderr, "STORE invalid index %d\n", idx);
                    vm->running = false;
                    break;
                }
                vm->memory[idx] = val;
                break;
            }
            case OP_LOAD: {
                if (vm->pc + 4 > vm->program_size) {
                    fprintf(stderr, "LOAD out of bounds\n");
                    vm->running = false;
                    break;
                }
                int32_t idx = read_i32_le(vm->program, vm->pc);
                vm->pc += 4;
                if (idx < 0 || idx >= MEM_SIZE) {
                    fprintf(stderr, "LOAD invalid index %d\n", idx);
                    vm->running = false;
                    break;
                }
                push(vm, vm->memory[idx]);
                break;
            }

            case OP_CALL: {
                if (vm->pc + 4 > vm->program_size) {
                    fprintf(stderr, "CALL out of bounds\n");
                    vm->running = false;
                    break;
                }
                uint32_t addr = (uint32_t)read_i32_le(vm->program, vm->pc);
                vm->pc += 4;
                // spec: push PC+1 to return stack and jump (we already consumed all bytes,
                // so return address is current pc)
                ret_push(vm, vm->pc);
                vm->pc = addr;
                break;
            }
            case OP_RET: {
                uint32_t addr = ret_pop(vm);
                vm->pc = addr;
                break;
            }

            case OP_HALTED: { // HALT
                vm->running = false;
                break;
            }

            default:
                fprintf(stderr, "Unknown opcode 0x%02X at pc=%u\n", opcode, vm->pc - 1);
                vm->running = false;
                break;
        }
    }
}
