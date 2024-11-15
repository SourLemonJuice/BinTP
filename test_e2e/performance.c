#include <stdbool.h>
#include <stdio.h>
#include <unistd.h> // emm... Never mind, I'm only testing on Linux anyway...

#include "bintp/bintp1.h"
#include "dump.h"

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

        Bintp1AppendField(&request.field, &field_sample);
        Bintp1AppendField(&request.field, &field_sample);

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

int main(void)
{
    TestRequestPerformance_(1000 * 1000, false, false);

    return 0;
}
