#include "vm.h"
#include <stdlib.h>
#include <stdio.h>

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
