#include "vm.h"
#include <stdlib.h>

static Obj *alloc_object(VM *vm, ObjType type) {
    Obj *obj = malloc(sizeof(Obj));
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

/* ===== SWEEP PHASE ===== */

static void sweep(VM *vm) {
    Obj **obj = &vm->heap;

    while (*obj) {
        if (!(*obj)->marked) {
            Obj *dead = *obj;
            *obj = dead->next;
            free(dead);
            vm->num_objects--;
        } else {
            (*obj)->marked = 0;
            obj = &(*obj)->next;
        }
    }
}
void gc(VM *vm) {
    mark_roots(vm);
    sweep(vm);
}
