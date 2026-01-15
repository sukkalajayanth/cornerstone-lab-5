#ifndef VM_H
#define VM_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    OBJ_PAIR,
    OBJ_FUNCTION,
    OBJ_CLOSURE
} ObjType;

typedef struct Obj Obj;

struct Obj {
    ObjType type;
    uint8_t marked;      /* GC mark bit */
    Obj *next;           /* heap linked list */

    union {
        struct { Obj *left, *right; } pair;
        struct { int dummy; } function;
        struct { Obj *function, *env; } closure;
    } as;
};

typedef enum {
    VAL_INT,
    VAL_OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        int i;
        Obj *obj;
    } as;
} Value;

#define STACK_MAX 1024

typedef struct {
    Value stack[STACK_MAX];
    int sp;

    Obj *heap;
    size_t num_objects;
} VM;

Obj *new_pair(VM *vm, Obj *a, Obj *b);
Obj *new_function(VM *vm);
Obj *new_closure(VM *vm, Obj *fn, Obj *env);

void push(VM *vm, Value v);
Value pop(VM *vm);

#endif
