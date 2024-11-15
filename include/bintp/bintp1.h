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

extern const uint8_t kBintp1MethodGet;
extern const uint8_t kBintp1MethodHead;
extern const uint8_t kBintp1MethodPost;
extern const uint8_t kBintp1MethodPut;
extern const uint8_t kBintp1MethodDelete;
extern const uint8_t kBintp1MethodConnect;
extern const uint8_t kBintp1MethodOptions;
extern const uint8_t kBintp1MethodTrace;

int Bintp1AppendField(int tgt_count, struct Bintp1FieldPair *tgt_fields[static tgt_count],
    struct Bintp1FieldPair new_field_ptr[static 1]);
int Bintp1SearchField(
    int field_count, struct Bintp1FieldPair fields[static field_count], size_t name_size, void *name_ptr);
int Bintp1SetField(
    int field_count, struct Bintp1FieldPair fields[static field_count], struct Bintp1FieldPair new[static 1]);

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