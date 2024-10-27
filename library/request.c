#include "request.h"

#include <stdint.h>
#include <stdio.h> // TODO debug
#include <stdlib.h>
#include <string.h>

void AddHeader(struct Request_ request[static 1], struct FieldPair_ new_field)
{
    request->field_count += 1;
    int count = request->field_count;
    struct FieldPair_ *field_list = request->fields;

    field_list = realloc(field_list, sizeof(struct FieldPair_[count]));
    field_list[count - 1] = new_field;

    request->fields = field_list;
}

/*
    Return the inserted content size
 */
static size_t InsertInfoString_(uint8_t *dest, char *str, size_t limit)
{
    int str_len = strlen(str);
    size_t str_size = sizeof(char[str_len]);

    memcpy(dest, str, str_size);
    memcpy(dest + str_size, &(char[]){'\r', '\n'}, 2);

    return str_size + 2;
}

static size_t InsertHeaderField_(uint8_t *dest, struct FieldPair_ field)
{
    size_t offset = 0;

    dest[offset] = field.name_size;
    offset += sizeof(uint8_t);

    memcpy(dest + offset, field.name, field.name_size);
    offset += field.name_size;

    dest[offset] = field.value_size;
    offset += sizeof(uint8_t);

    memcpy(dest + offset, field.value, field.value_size);
    offset += field.value_size;

    return offset;
}

struct MemPair_ GenerateRequest(struct Request_ prepare)
{
    size_t info_size = 0;
    info_size += sizeof(char[strlen(prepare.version) + 2]);
    info_size += sizeof(char[strlen(prepare.resource_id) + 2]);

    size_t header_size = 0;
    for (int i = 0; i < prepare.field_count; i++) {
        header_size += sizeof(uint8_t) + prepare.fields[i].name_size;
        header_size += sizeof(uint8_t) + prepare.fields[i].value_size;
    }

    size_t size = info_size + header_size + prepare.load_size;
    uint8_t *request = malloc(size);

    size_t offset = 0;
    offset += InsertInfoString_(request + offset, prepare.version, size);
    offset += InsertInfoString_(request + offset, prepare.resource_id, size);

    printf("info end in:\t%zu\n", offset); // TODO debug

    for (int i = 0; i < prepare.field_count; i++)
        offset += InsertHeaderField_(request + offset, prepare.fields[i]);

    printf("header end in:\t%zu\n", offset); // TODO debug

    memcpy(request + offset, prepare.load, prepare.load_size);
    if (offset + prepare.load_size != size)
        return (struct MemPair_){.ptr = NULL};

    return (struct MemPair_){.size = size, .ptr = request};
}
