#include "request.h"

#include <stdint.h>
#include <stdio.h> // TODO just for debug
#include <stdlib.h>
#include <string.h>

#define HEADER_END_FIELD \
    (struct BintpFieldPair) \
    { \
        .name_size = 0, .value_size = 0 \
    }

void BintpAddHeader(struct BintpRequest request[static 1], struct BintpFieldPair *new_field_ptr)
{
    struct BintpFieldPair new_field = *new_field_ptr;

    request->field_count += 1;
    int count = request->field_count;
    struct BintpFieldPair *field_list = request->fields;

    field_list = realloc(field_list, sizeof(struct BintpFieldPair[count]));
    field_list[count - 1] = new_field;

    request->fields = field_list;
}

void BintpFreeUpHeader(struct BintpRequest request[static 1])
{
    request->field_count = 0;
    free(request->fields);
}

/*
    Return the inserted content size
 */
static size_t InsertInfoString_(void *dest, char *str, size_t limit)
{
    int str_len = strlen(str);
    size_t str_size = sizeof(char[str_len]);

    memcpy(dest, str, str_size);
    memcpy(dest + str_size, &(char[]){'\r', '\n'}, 2);

    return str_size + 2;
}

// TODO use void pointer
/*
    If field.name_size == 0, then just insert one byte with zero.
 */
static size_t InsertHeaderField_(uint8_t *dest, struct BintpFieldPair field)
{
    size_t offset = 0;

    dest[offset] = field.name_size;
    offset += sizeof(uint8_t);
    if (field.name_size == 0)
        return offset;

    memcpy(dest + offset, field.name, field.name_size);
    offset += field.name_size;

    dest[offset] = field.value_size;
    offset += sizeof(uint8_t);

    memcpy(dest + offset, field.value, field.value_size);
    offset += field.value_size;

    return offset;
}

struct MemPair BintpGenerateRequest(struct BintpRequest *prepare_ptr)
{
    struct BintpRequest prepare = *prepare_ptr;

    size_t size = 0;                               // theoretical size
    size += sizeof(uint8_t);                       // version
    size += sizeof(char[strlen(prepare.uri) + 2]); // URI

    for (int i = 0; i < prepare.field_count; i++) {
        size += sizeof(uint8_t) + prepare.fields[i].name_size;
        size += sizeof(uint8_t) + prepare.fields[i].value_size;
    }
    size += sizeof(uint8_t);

    size += prepare.load_size;

    void *request = malloc(size);

    size_t offset = 0;
    *(uint8_t *)request = prepare.version;
    offset += sizeof(uint8_t);

    offset += InsertInfoString_(request + offset, prepare.uri, size);

    printf("info end in:\t%zu\n", offset); // TODO debug

    for (int i = 0; i < prepare.field_count; i++)
        offset += InsertHeaderField_(request + offset, prepare.fields[i]);
    offset += InsertHeaderField_(request + offset, HEADER_END_FIELD);

    printf("header end in:\t%zu\n", offset); // TODO debug

    memcpy(request + offset, prepare.load, prepare.load_size);
    if (offset + prepare.load_size != size)
        return (struct MemPair){.ptr = NULL};

    return (struct MemPair){.size = size, .ptr = request};
}
