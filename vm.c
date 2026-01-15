#include "vm.h"
#include <stdlib.h>
#include <stdio.h>

#define MARK_STACK_MAX 1000000

static Obj *alloc_object(VM *vm, ObjType type) {
    Obj *obj = malloc(sizeof(Obj));
    if (!obj) exit(1);

    obj->type = type;
    obj->marked = 0;
    obj->next = vm->heap;
    vm->heap = obj;
    vm->num_objects++;

    return obj;
}

Obj *new_pair(VM *vm, Obj *a, Obj *b) {
    Obj *obj = alloc_object(vm, OBJ_PAIR);
    obj->as.pair.left = a;
    obj->as.pair.right = b;
    return obj;
}

Obj *new_function(VM *vm) {
    return alloc_object(vm, OBJ_FUNCTION);
}

Obj *new_closure(VM *vm, Obj *fn, Obj *env) {
    Obj *obj = alloc_object(vm, OBJ_CLOSURE);
    obj->as.closure.function = fn;
    obj->as.closure.env = env;
    return obj;
}

void push(VM *vm, Value v) {
    vm->stack[vm->sp++] = v;
}

Value pop(VM *vm) {
    return vm->stack[--vm->sp];
}

/* ===== MARK PHASE ===== */

static void mark_object(Obj *root) {
    if (!root) return;

    Obj *stack[MARK_STACK_MAX];
    int top = 0;
    stack[top++] = root;

    while (top > 0) {
        Obj *obj = stack[--top];
        if (obj->marked) continue;

        obj->marked = 1;

        switch (obj->type) {
            case OBJ_PAIR:
                if (obj->as.pair.left)  stack[top++] = obj->as.pair.left;
                if (obj->as.pair.right) stack[top++] = obj->as.pair.right;
                break;
            case OBJ_CLOSURE:
                stack[top++] = obj->as.closure.function;
                stack[top++] = obj->as.closure.env;
                break;
            default:
                break;
        }
    }
}

static void mark_roots(VM *vm) {
    for (int i = 0; i < vm->sp; i++) {
        if (vm->stack[i].type == VAL_OBJ)
            mark_object(vm->stack[i].as.obj);
    }
}
