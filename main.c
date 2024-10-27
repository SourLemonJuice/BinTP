#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "library/request.h" // TODO

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

int main(void)
{
    size_t site = 0;
    struct Request_ request = {
        .version = "BinTP/0",
        .resource_id = "/",
    };

    AddHeader(&request, (struct FieldPair_){
                            .name_size = 1,
                            .name = &(uint8_t[]){0x31},
                            .value_size = 2,
                            .value = &(uint8_t[]){0x31, 0x99},
                        });

    request.load_size = 2;
    request.load = &(uint8_t[]){0xff, 0xff};

    struct MemPair_ pkg = GenerateRequest(request);
    printf("Size: %zu\n", pkg.size);
    DumpHex(pkg.ptr, pkg.size);

    return 0;
}
