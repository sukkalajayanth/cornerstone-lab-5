#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define STACK_MAX 1024
#define MEM_SIZE 1024
#define RET_STACK_MAX 256
#define PROGRAM_MAX 65536

// Opcodes as per lab spec
enum {
    OP_PUSH = 0x01,
    OP_POP  = 0x02,
    OP_DUP  = 0x03,

    OP_ADD  = 0x10,
    OP_SUB  = 0x11,
    OP_MUL  = 0x12,
    OP_DIV  = 0x13,
    OP_CMP  = 0x14,

    OP_JMP  = 0x20,
    OP_JZ   = 0x21,
    OP_JNZ  = 0x22,

    OP_STORE = 0x30,
    OP_LOAD  = 0x31,
    OP_CALL  = 0x40,
    OP_RET   = 0x41,

    OP_HALTED = 0xFF // HALT
};

typedef struct {
    int32_t stack[STACK_MAX];
    int32_t sp; // index of top (sp = -1 means empty)

    int32_t ret_stack[RET_STACK_MAX];
    int32_t rsp;

    int32_t memory[MEM_SIZE];

    uint8_t program[PROGRAM_MAX];
    size_t program_size;

    uint32_t pc;
    bool running;
} VM;

void vm_init(VM *vm);
bool vm_load_program(VM *vm, const char *filename);
void vm_run(VM *vm);
void vm_dump_stack(const VM *vm);
void gc(VM *vm);

#endif // VM_H
