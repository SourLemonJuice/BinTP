#include "dump.h"

#include <stdint.h>
#include <stdio.h>

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

void DumpBintpFieldPair(struct Bintp1FieldPair field[static 1])
{
    printf("name_size:\t%zu\n", field->name_size);
    DumpHex(field->name, field->name_size);

    printf("value_size:\t%zu\n", field->value_size);
    DumpHex(field->value, field->value_size);
}
