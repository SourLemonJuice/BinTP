#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bintp1.h"

void DumpHex(void *ptr, size_t size)
{
    uint8_t *now_ptr = (uint8_t *)ptr;
    for (int i = 0; i < size; i++) {
        printf("%02x", *now_ptr);
        if ((i + 1) % 2 == 0)
            printf(" ");
        now_ptr++;
    }
    printf("\n");
}

void DumpBintpField(struct BintpFieldPair *field)
{
    printf("name_size:\t%zu\n", field->name_size);
    DumpHex(field->name, field->name_size);

    printf("value_size:\t%zu\n", field->value_size);
    DumpHex(field->value, field->value_size);
}

static void TestRequest_(void)
{
    struct BintpRequest request = {
        .version = 0xff,
        .method = 0xee,
        .uri = "/",
    };

    BintpAddHeader(&request.field_count, &request.fields,
        &(struct BintpFieldPair){
            .name_size = 1,
            .name = &(uint8_t[]){0x31},
            .value_size = 160,
            .value = &(uint8_t[]){0x31, 0x99},
        });

    struct MemPair pkg = BintpGenerateRequest(&request);
    BintpFreeUpHeader(&request);

    if (MemPairIsNull(&pkg) == true) {
        printf("pkg is NULL\n");
        exit(EXIT_FAILURE);
    }

    printf("Size: %zu\n", pkg.size);
    DumpHex(pkg.ptr, pkg.size);

    printf("== == ==\n");
    printf("BintpParseVersion():\t%d\n", BintpParseVersion(pkg.ptr, pkg.size));

    struct BintpRequest parsed_request = {0};
    printf("Header size:\t%zu\n", BintpParseRequest(pkg.ptr, pkg.size, &parsed_request));

    printf("URI:\t%s\n", parsed_request.uri);
    printf("Method:\t%u\n", parsed_request.method);

    for (int i = 0; i < parsed_request.field_count; i++)
        DumpBintpField(&parsed_request.fields[i]);
}

static void TestResponse(void)
{
    struct BintpResponse response = {
        .version = 1,
        .status = 0xabab,
    };

    struct MemPair pkg = BintpGenerateResponse(&response);
    free(response.fields);

    if (MemPairIsNull(&pkg) == true)
        exit(EXIT_FAILURE);

    printf("response size:\t%zu\n", pkg.size);
    DumpHex(pkg.ptr, pkg.size);
}

int main(void)
{
    TestRequest_();
    printf("---- ---- ----\n");
    // TestResponse();

    return 0;
}
