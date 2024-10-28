#ifndef BINTP_REQUEST_H_
#define BINTP_REQUEST_H_

#include <stdint.h>
#include <stdlib.h>

/*
    Change structures name
 */

struct MemPair {
    size_t size;
    void *ptr;
};

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
    char *version;
    char *resource_id; // TODO -> uri
    int field_count;
    struct BintpFieldPair *fields;
    size_t load_size;
    void *load;
};

void BintpAddHeader(struct BintpRequest request[static 1], struct BintpFieldPair *new_field_ptr);
void BintpFreeUpHeader(struct BintpRequest request[static 1]);
struct MemPair BintpGenerateRequest(struct BintpRequest *prepare_ptr);

#endif
