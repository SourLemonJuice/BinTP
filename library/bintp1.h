#ifndef BINTP_BINTP1_H_
#define BINTP_BINTP1_H_

#include <stdint.h>
#include <stdlib.h>

/*
    Size is the number of bytes
 */
struct Bintp1FieldPair {
    size_t name_size;
    void *name;
    size_t value_size;
    void *value;
};

struct Bintp1Request {
    uint8_t method;
    char *uri;
    int field_count;
    struct Bintp1FieldPair *fields;
};

struct Bintp1Response {
    uint16_t status;
    int field_count;
    struct Bintp1FieldPair *fields;
};

void Bintp1AppendField(int *tgt_count, struct Bintp1FieldPair *tgt_fields[static * tgt_count],
    struct Bintp1FieldPair new_field_ptr[static 1]);

size_t Bintp1CalcRequestSize(struct Bintp1Request prepare_ptr[static 1]);
size_t Bintp1WriteRequest(void *dest, size_t limit, struct Bintp1Request prepare_ptr[static 1]);
void Bintp1FreeUpRequest(struct Bintp1Request form[static 1]);
size_t Bintp1CalcResponseSize(struct Bintp1Response prepare_ptr[static 1]);
size_t Bintp1WriteResponse(void *dest, size_t limit, struct Bintp1Response prepare_ptr[static 1]);
void Bintp1FreeUpResponse(struct Bintp1Response form[static 1]);

int BintpParseVersion(void *bin, size_t bin_size);
size_t Bintp1ParseRequest(void *bin, size_t bin_size, struct Bintp1Request form[static 1]);
size_t Bintp1ParseResponse(void *bin, size_t bin_size, struct Bintp1Response form[static 1]);

#endif
