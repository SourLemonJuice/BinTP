#ifndef BINTP_BINTP1_H_
#define BINTP_BINTP1_H_

#include <stdint.h>
#include <stdlib.h>

#include "memory.h"

/*
    Size is the number of bytes
 */
struct BintpFieldPair {
    size_t name_size;
    void *name;
    size_t value_size;
    void *value;
};

struct BintpRequest {
    uint8_t version;
    uint8_t method;
    char *uri;
    int field_count;
    struct BintpFieldPair *fields;
};

struct BintpResponse {
    uint8_t version;
    uint16_t status;
    int field_count;
    struct BintpFieldPair *fields;
};

void BintpAddHeader(int *tgt_count, struct BintpFieldPair **tgt_fields, struct BintpFieldPair new_field_ptr[static 1]);
void BintpFreeUpHeader(struct BintpRequest request[static 1]);
struct MemPair BintpGenerateRequest(struct BintpRequest *prepare_ptr);
struct MemPair BintpGenerateResponse(struct BintpResponse prepare_ptr[static 1]);
int BintpParseVersion(void *bin, size_t bin_size);
size_t BintpParseRequest(void *bin, size_t bin_size, struct BintpRequest form[static 1]);
size_t BintpParseResponse(void *bin, size_t bin_size, struct BintpResponse form[static 1]);

#endif
