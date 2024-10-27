#pragma once // TODO change to ifndef

#include <stdint.h>
#include <stdlib.h>

struct MemPair_ {
    size_t size;
    void *ptr;
};

/*
    Size is the number of bytes
 */
struct FieldPair_ {
    uint8_t name_size;
    void *name;
    uint8_t value_size;
    void *value;
};

struct Request_ {
    char *version;
    char *resource_id; // TODO -> uri
    int field_count;
    struct FieldPair_ *fields;
    int load_size;
    void *load;
};

void AddHeader(struct Request_ request[static 1], struct FieldPair_ new_field);
struct MemPair_ GenerateRequest(struct Request_ prepare);
