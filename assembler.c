#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "vm.h"

#define MAX_LINE 256
#define MAX_LABELS 256

typedef struct {
    char name[64];
    uint32_t addr;
} Label;

typedef struct {
    Label labels[MAX_LABELS];
    int count;
} LabelTable;

static void labels_init(LabelTable *t) {
    t->count = 0;
}

static void labels_add(LabelTable *t, const char *name, uint32_t addr) {
    if (t->count >= MAX_LABELS) {
        fprintf(stderr, "Too many labels\n");
        exit(1);
    }
    strncpy(t->labels[t->count].name, name, sizeof(t->labels[t->count].name) - 1);
    t->labels[t->count].name[sizeof(t->labels[t->count].name) - 1] = '\0';
    t->labels[t->count].addr = addr;
    t->count++;
}

static int labels_find(const LabelTable *t, const char *name, uint32_t *addr_out) {
    for (int i = 0; i < t->count; ++i) {
        if (strcmp(t->labels[i].name, name) == 0) {
            *addr_out = t->labels[i].addr;
            return 1;
        }
    }
    return 0;
}

static void write_i32_le(FILE *f, int32_t v) {
    uint8_t b[4];
    b[0] = (uint8_t)(v & 0xFF);
    b[1] = (uint8_t)((v >> 8) & 0xFF);
    b[2] = (uint8_t)((v >> 16) & 0xFF);
    b[3] = (uint8_t)((v >> 24) & 0xFF);
    fwrite(b, 1, 4, f);
}

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static void remove_comment(char *line) {
    char *p = strchr(line, ';');
    if (p) *p = '\0';
}

static int is_label(const char *token) {
    size_t len = strlen(token);
    return len > 0 && token[len - 1] == ':';
}

static void strip_colon(char *token) {
    size_t len = strlen(token);
    if (len > 0 && token[len - 1] == ':') token[len - 1] = '\0';
}

static uint8_t opcode_from_mnemonic(const char *mn) {
    if (strcmp(mn, "PUSH") == 0) return OP_PUSH;
    if (strcmp(mn, "POP")  == 0) return OP_POP;
    if (strcmp(mn, "DUP")  == 0) return OP_DUP;

    if (strcmp(mn, "ADD")  == 0) return OP_ADD;
    if (strcmp(mn, "SUB")  == 0) return OP_SUB;
    if (strcmp(mn, "MUL")  == 0) return OP_MUL;
    if (strcmp(mn, "DIV")  == 0) return OP_DIV;
    if (strcmp(mn, "CMP")  == 0) return OP_CMP;

    if (strcmp(mn, "JMP")  == 0) return OP_JMP;
    if (strcmp(mn, "JZ")   == 0) return OP_JZ;
    if (strcmp(mn, "JNZ")  == 0) return OP_JNZ;

    if (strcmp(mn, "STORE") == 0) return OP_STORE;
    if (strcmp(mn, "LOAD")  == 0) return OP_LOAD;
    if (strcmp(mn, "CALL")  == 0) return OP_CALL;
    if (strcmp(mn, "RET")   == 0) return OP_RET;

    if (strcmp(mn, "HALT")  == 0) return OP_HALTED;

    fprintf(stderr, "Unknown mnemonic '%s'\n", mn);
    exit(1);
}

static int mnemonic_needs_imm(uint8_t op) {
    switch (op) {
        case OP_PUSH:
        case OP_JMP:
        case OP_JZ:
        case OP_JNZ:
        case OP_STORE:
        case OP_LOAD:
        case OP_CALL:
            return 1;
        default:
            return 0;
    }
}

// First pass: compute addresses of labels
static void first_pass(FILE *in, LabelTable *labels) {
    labels_init(labels);
    char line[MAX_LINE];
    uint32_t pc = 0;

    while (fgets(line, sizeof(line), in)) {
        remove_comment(line);
        char *p = trim(line);
        if (*p == '\0') continue;

        char *save = NULL;
        char *token = strtok_r(p, " \t\r\n", &save);
        if (!token) continue;

        // Label
        if (is_label(token)) {
            strip_colon(token);
            labels_add(labels, token, pc);
            token = strtok_r(NULL, " \t\r\n", &save);
            if (!token) continue;
        }

        uint8_t op = opcode_from_mnemonic(token);
        pc += 1;
        if (mnemonic_needs_imm(op)) {
            pc += 4;
        }
    }
}

// Second pass: emit bytecode
static void second_pass(FILE *in, FILE *out, const LabelTable *labels) {
    char line[MAX_LINE];
    uint32_t pc = 0;

    while (fgets(line, sizeof(line), in)) {
        remove_comment(line);
        char *p = trim(line);
        if (*p == '\0') continue;

        char *save = NULL;
        char *token = strtok_r(p, " \t\r\n", &save);
        if (!token) continue;

        // Labels
        if (is_label(token)) {
            token = strtok_r(NULL, " \t\r\n", &save);
            if (!token) continue;
        }

        uint8_t op = opcode_from_mnemonic(token);
        fwrite(&op, 1, 1, out);
        pc += 1;

        if (mnemonic_needs_imm(op)) {
            char *arg = strtok_r(NULL, " \t\r\n", &save);
            if (!arg) {
                fprintf(stderr, "Expected operand for mnemonic needing immediate\n");
                exit(1);
            }

            // label or number?
            uint32_t addr;
            char *endptr = NULL;
            long val = strtol(arg, &endptr, 0);
            if (*endptr == '\0') {
                // numeric
                write_i32_le(out, (int32_t)val);
            } else {
                // treat as label
                if (!labels_find(labels, arg, &addr)) {
                    fprintf(stderr, "Unknown label '%s'\n", arg);
                    exit(1);
                }
                write_i32_le(out, (int32_t)addr);
            }
            pc += 4;
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output.bin>\n", argv[0]);
        return 1;
    }

    const char *in_name = argv[1];
    const char *out_name = argv[2];

    FILE *in = fopen(in_name, "r");
    if (!in) {
        perror("fopen input");
        return 1;
    }
    LabelTable labels;

    first_pass(in, &labels);
    rewind(in);

    FILE *out = fopen(out_name, "wb");
    if (!out) {
        perror("fopen output");
        fclose(in);
        return 1;
    }

    second_pass(in, out, &labels);

    fclose(in);
    fclose(out);
    return 0;
}
