#ifndef BINTP_REQUEST_H_
#define BINTP_REQUEST_H_

#include <stdint.h>
#include <stdlib.h>

#include "memory.h"

/*
    Size is the number of bytes
 */
struct BintpFieldPair {
    uint8_t name_size;
    void *name;
    uint8_t value_size;
    void *value;
};

struct BintpRequest {
    uint8_t version;
    char *uri;
    int field_count;
    struct BintpFieldPair *fields;
    size_t load_size;
    void *load;
};

void BintpAddHeader(struct BintpRequest request[static 1], struct BintpFieldPair *new_field_ptr);
void BintpFreeUpHeader(struct BintpRequest request[static 1]);
struct MemPair BintpGenerateRequest(struct BintpRequest *prepare_ptr);
int BintpParseVersion(void *bin, size_t bin_size);
size_t BintpParseRequest(void *bin, size_t bin_size, struct BintpRequest form[static 1]);

#endif
