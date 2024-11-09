#ifndef BINTP_BINTP1_H_
#define BINTP_BINTP1_H_

#include <stdint.h>
#include <stdlib.h>

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
    uint8_t version; // TODO to be delete
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

void Bintp1AppendField(int *tgt_count, struct BintpFieldPair *tgt_fields[static * tgt_count],
    struct BintpFieldPair new_field_ptr[static 1]);

size_t Bintp1CalcRequestSize(struct BintpRequest prepare_ptr[static 1]);
size_t Bintp1WriteRequest(void *dest, size_t limit, struct BintpRequest prepare_ptr[static 1]);
void Bintp1FreeUpRequest(struct BintpRequest form[static 1]);
size_t Bintp1CalcResponseSize(struct BintpResponse prepare_ptr[static 1]);
size_t Bintp1WriteResponse(void *dest, size_t limit, struct BintpResponse prepare_ptr[static 1]);
void Bintp1FreeUpResponse(struct BintpResponse form[static 1]);

int BintpParseVersion(void *bin, size_t bin_size);
size_t BintpParseRequest(void *bin, size_t bin_size, struct BintpRequest form[static 1]);
size_t BintpParseResponse(void *bin, size_t bin_size, struct BintpResponse form[static 1]);

#endif
