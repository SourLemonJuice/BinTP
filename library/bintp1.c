#include "bintp1.h"

#include <iso646.h>
#include <stdio.h> // TODO just for debug
#include <string.h>

#define HEADER_END_FIELD \
    (struct BintpFieldPair) \
    { \
        .name_size = 0, .value_size = 0 \
    }

void BintpAddHeader(int *tgt_count, struct BintpFieldPair **tgt_fields, struct BintpFieldPair new_field_ptr[static 1])
{
    struct BintpFieldPair new_field = *new_field_ptr;

    *tgt_count += 1;
    int count = *tgt_count;
    struct BintpFieldPair *field_list = *tgt_fields;

    field_list = realloc(field_list, sizeof(struct BintpFieldPair[count]));
    field_list[count - 1] = new_field;

    *tgt_fields = field_list;
}

// TODO TBD
void BintpFreeUpHeader(struct BintpRequest request[static 1])
{
    request->field_count = 0;
    free(request->fields);
}

static size_t GetFieldsSize_(int count, struct BintpFieldPair fields[static count])
{
    size_t size = 0;

    for (int i = 0; i < count; i++) {
        size += 1 + fields[i].name_size;
        size += 1 + fields[i].value_size;
    }

    return size;
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
static size_t InsertSingleField_(uint8_t *dest, struct BintpFieldPair field)
{
    size_t offset = 0;

    dest[offset] = field.name_size;
    offset += 1;
    if (field.name_size == 0)
        return offset;

    memcpy(dest + offset, field.name, field.name_size);
    offset += field.name_size;

    dest[offset] = field.value_size;
    offset += 1;

    memcpy(dest + offset, field.value, field.value_size);
    offset += field.value_size;

    return offset;
}

static size_t InsertFields_(void *dest, int count, struct BintpFieldPair fields[static count])
{
    size_t offset = 0;

    for (int i = 0; i < count; i++)
        offset += InsertSingleField_(dest + offset, fields[i]);
    offset += InsertSingleField_(dest + offset, HEADER_END_FIELD);

    return offset;
}

struct MemPair BintpGenerateRequest(struct BintpRequest *prepare_ptr)
{
    struct BintpRequest prepare = *prepare_ptr;

    size_t size = 0;                               // theoretical size
    size += 1 + 1;                                 // version + method
    size += sizeof(char[strlen(prepare.uri) + 2]); // URI

    size += GetFieldsSize_(prepare.field_count, prepare.fields);
    size += 1;

    size += prepare.load_size;

    void *request = malloc(size);

    size_t offset = 0;
    *(uint8_t *)request = prepare.version;
    offset += 1;

    *(uint8_t *)(request + offset) = prepare.method;
    offset += 1;

    offset += InsertInfoString_(request + offset, prepare.uri, size);

    printf("info end in:\t%zu\n", offset); // TODO debug

    offset += InsertFields_(request + offset, prepare.field_count, prepare.fields);

    printf("header end in:\t%zu\n", offset); // TODO debug

    memcpy(request + offset, prepare.load, prepare.load_size);
    if (offset + prepare.load_size != size)
        return MEMPAIR_NULL;

    return (struct MemPair){.size = size, .ptr = request};
}

struct MemPair BintpGenerateResponse(struct BintpResponse prepare_ptr[static 1])
{
    struct BintpResponse prepare = *prepare_ptr;

    size_t size = 0;
    size += 1 + 2; // version + status code
    size += GetFieldsSize_(prepare.field_count, prepare.fields);
    size += 1 + prepare.load_size;

    void *response = malloc(size);

    size_t offset = 0;
    *(uint8_t *)(response + offset) = prepare.version;
    offset += 1;

    *(uint16_t *)(response + offset) = prepare.status;
    offset += 2;

    offset += InsertFields_(response + offset, prepare.field_count, prepare.fields);

    memcpy(response + offset, prepare.load, prepare.load_size);
    if (offset + prepare.load_size != size)
        return MEMPAIR_NULL;

    return (struct MemPair){.size = size, .ptr = response};
}

/*
    Parser
 */

int BintpParseVersion(void *bin, size_t bin_size)
{
    return *(uint8_t *)bin;
}

static size_t FindInfoStringRange_(char *str_start, size_t max_size)
{
    char *end = strnstr(str_start, "\r\n", max_size); // TODO uri shouldn't have \0 but still not be appropriate
    if (end == NULL)
        return 0;

    return (end - str_start) + sizeof(char[2]);
}

/*
    Return a standard C string
 */
static char *ParseInfoString_(char *str_start, size_t str_range)
{
    size_t cstr_size = str_range - 2 + 1;
    char *str_buffer = malloc(sizeof(char[cstr_size]));
    if (str_buffer == NULL)
        return 0;

    memcpy(str_buffer, str_start, cstr_size);
    str_buffer[cstr_size] = '\0';

    return str_buffer;
}

/*
    TODO if *_size is 0, need throw an error
 */
static size_t ParseSingleField_(void *bin, size_t bin_size, struct BintpFieldPair pair[static 1])
{
    size_t offset = 0;

    size_t name_size = *(uint8_t *)(bin + offset);
    pair->name_size = name_size;
    offset += 1;

    void *name = malloc(name_size);
    memcpy(name, bin + offset, name_size);
    pair->name = name;
    offset += name_size;

    size_t value_size = *(uint8_t *)(bin + offset);
    pair->value_size = value_size;
    offset += 1;

    void *value = malloc(value_size);
    memcpy(value, bin + offset, value_size);
    pair->value = value;
    offset += value_size;

    return offset;
}

static size_t ParseFields_(
    void *bin, size_t bin_size, int tgt_count[static 1], struct BintpFieldPair **tgt_list)
{
    size_t offset = 0;
    int count = 0;
    struct BintpFieldPair *list_ptr = NULL;

    while (true) {
        if (bin_size >= 1 and *(uint8_t *)(bin + offset) == 0) {
            offset += 1;
            break;
        }
        count += 1;
        list_ptr = realloc(list_ptr, sizeof(struct BintpFieldPair[count]));

        offset += ParseSingleField_(bin + offset, bin_size - offset, list_ptr + count - 1);
    }

    *tgt_count = count;
    *tgt_list = list_ptr;
    return offset;
}

/*
    Just for version 1

    Parses only the header, not the load
 */
size_t BintpParseRequest(void *bin, size_t bin_size, struct BintpRequest form[static 1])
{
    form->version = 1;
    size_t offset = 1;

    form->method = *(uint8_t *)(bin + offset);
    offset += 1;

    size_t uri_range = FindInfoStringRange_(bin + offset, bin_size - offset);
    form->uri = ParseInfoString_(bin + offset, uri_range);
    offset += uri_range;

    offset += ParseFields_(bin + offset, bin_size - offset, &form->field_count, &form->fields);

    return offset;
}

size_t BintpParseResponse(void *bin, size_t bin_size, struct BintpResponse form[static 1])
{
    form->version = 1;
    size_t offset = 1;

    form->status = *(uint16_t *)(bin + offset);
    offset += 2;

    offset += ParseFields_(bin + offset, bin_size - offset, &form->field_count, &form->fields);

    return offset;
}
