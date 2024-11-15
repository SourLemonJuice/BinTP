#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bintp/bintp1.h"
#include "dump.h"

static void TestRequest_(void)
{
    struct Bintp1Request request = {
        .method = kBintp1MethodGet,
        .uri = "/",
    };

    Bintp1AppendField(&request.field, &(struct Bintp1FieldPair){
                                          .name_size = 1,
                                          .name = &(uint8_t[]){0x31},
                                          .value_size = 10,
                                          .value = &(uint8_t[10]){0x31, 0},
                                      });

    size_t bin_size = Bintp1CalcRequestSize(&request);
    if (bin_size == 0) {
        printf("pkg is NULL\n");
        exit(EXIT_FAILURE);
    }
    printf("Size: %zu\n", bin_size);

    void *bin_ptr = malloc(bin_size);
    Bintp1WriteRequest(bin_ptr, bin_size, &request);
    Bintp1FreeUpRequest(&request);
    DumpHex(bin_ptr, bin_size);

    printf("== == ==\n");
    printf("BintpParseVersion():\t%d\n", BintpParseVersion(bin_ptr, bin_size));

    struct Bintp1Request parsed_request = {0};
    printf("Header size:\t%zu\n", Bintp1ParseRequest(bin_ptr, bin_size, &parsed_request));

    printf("URI:\t%s\n", parsed_request.uri);
    printf("Method:\t%u\n", parsed_request.method);

    for (int i = 0; i < parsed_request.field.count; i++)
        DumpBintpFieldPair(&parsed_request.field.pairs[i]);
}

static void TestResponse(void)
{
    struct Bintp1Response response = {
        .status = 200,
    };

    size_t bin_size = Bintp1CalcResponseSize(&response);
    if (bin_size == 0)
        exit(EXIT_FAILURE);
    void *bin_ptr = malloc(bin_size);
    Bintp1WriteResponse(bin_ptr, bin_size, &response);
    Bintp1FreeUpResponse(&response);

    printf("response size:\t%zu\n", bin_size);
    DumpHex(bin_ptr, bin_size);
}

int main(void)
{
    TestRequest_();
    printf("---- ---- ----\n");
    TestResponse();

    return 0;
}
