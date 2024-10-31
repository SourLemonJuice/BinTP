#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "request.h" // TODO

void DumpHex(void *ptr, size_t size) {
    uint8_t *now_ptr = (uint8_t *)ptr;
    for (int i = 0; i < size; i++) {
        printf("%02x", *now_ptr);
        if ((i + 1) % 2 == 0)
            printf(" ");
        now_ptr++;
    }
    printf("\n");
}

void DumpBintpField(struct BintpFieldPair *field) {
    printf("name_size:\t%d\n", field->name_size);
    DumpHex(field->name, field->name_size);

    printf("value_size:\t%d\n", field->value_size);
    DumpHex(field->value, field->value_size);
}

int main(void) {
    size_t site = 0;
    struct BintpRequest request = {
        .version = 0,
        .method = 876,
        .uri = "/",
    };

    BintpAddHeader(&request, &(struct BintpFieldPair){
                                 .name_size = 1,
                                 .name = &(uint8_t[]){0x31},
                                 .value_size = 2,
                                 .value = &(uint8_t[]){0x31, 0x99},
                             });

    request.load_size = 2;
    request.load = &(uint8_t[]){0xff, 0xff};

    struct MemPair pkg = BintpGenerateRequest(&request);
    BintpFreeUpHeader(&request);

    if (MemPairIsNull(&pkg) == true) {
        printf("pkg is NULL\n");
        return 1;
    }

    printf("Size: %zu\n", pkg.size);
    DumpHex(pkg.ptr, pkg.size);

    printf("== == ==\n");
    printf("Version:\t%d\n", BintpParseVersion(pkg.ptr, pkg.size));

    struct BintpRequest parsed_request = {0};
    printf("Header size:\t%zu\n", BintpParseRequest(pkg.ptr, pkg.size, &parsed_request));

    printf("URI:\t%s\n", parsed_request.uri);
    printf("Method:\t%u\n", parsed_request.method);

    for (int i = 0; i < parsed_request.field_count; i++)
        DumpBintpField(&parsed_request.fields[i]);

    return 0;
}