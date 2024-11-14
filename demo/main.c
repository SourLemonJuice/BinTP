#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // emm... Never mind, I'm only testing on Linux anyway...

#include "bintp1.h"

static void DumpHex_(void *ptr, size_t size)
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

static void DumpBintpField_(struct Bintp1FieldPair *field)
{
    printf("name_size:\t%zu\n", field->name_size);
    DumpHex_(field->name, field->name_size);

    printf("value_size:\t%zu\n", field->value_size);
    DumpHex_(field->value, field->value_size);
}

static void TestRequest_(void)
{
    struct Bintp1Request request = {
        .method = kBintp1MethodGet,
        .uri = "/",
    };

    Bintp1AppendField(request.field_count, &request.fields,
        &(struct Bintp1FieldPair){
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
    DumpHex_(bin_ptr, bin_size);

    printf("== == ==\n");
    printf("BintpParseVersion():\t%d\n", BintpParseVersion(bin_ptr, bin_size));

    struct Bintp1Request parsed_request = {0};
    printf("Header size:\t%zu\n", Bintp1ParseRequest(bin_ptr, bin_size, &parsed_request));

    printf("URI:\t%s\n", parsed_request.uri);
    printf("Method:\t%u\n", parsed_request.method);

    for (int i = 0; i < parsed_request.field_count; i++)
        DumpBintpField_(&parsed_request.fields[i]);
}

static void TestRequestPerformance_(int cycle, bool print_toggle, bool pause_toggle)
{
    struct Bintp1FieldPair field_sample = {
        .name = &(uint8_t[7]){0},
        .name_size = 7,
        .value = &(uint8_t[20]){0},
        .value_size = 20,
    };

    for (int i = 0; i < cycle; i++) {
        struct Bintp1Request request = {
            .method = 0xee,
            .uri = "/",
        };

        Bintp1AppendField(request.field_count, &request.fields, &field_sample);
        Bintp1AppendField(request.field_count, &request.fields, &field_sample);

        size_t bin_size = Bintp1CalcRequestSize(&request);
        if (bin_size == 0)
            exit(EXIT_FAILURE);
        void *bin_ptr = malloc(bin_size);
        Bintp1WriteRequest(bin_ptr, bin_size, &request);

        if (print_toggle == true) {
            printf("[%d]:\tsize->%zu\n", i, bin_size);
            printf("==== ==== ====\n");
        }

        Bintp1FreeUpRequest(&request);
        free(bin_ptr);
    }

    if (pause_toggle == true)
        pause(); // testing memory leak

    printf("Request performance test run successfully with: samples=%d, print=%d\n", cycle, print_toggle);
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
    DumpHex_(bin_ptr, bin_size);
}

int main(void)
{
    TestRequest_();
    printf("---- ---- ----\n");
    TestResponse();

    // TestRequestPerformance_(1000 * 1000, false, false);

    return 0;
}
