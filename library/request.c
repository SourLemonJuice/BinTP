#include "request.h"

#include <stdio.h> // TODO just for debug
#include <string.h>
#include <iso646.h>

#define HEADER_END_FIELD \
    (struct BintpFieldPair) { \
        .name_size = 0, .value_size = 0 \
    }

void BintpAddHeader(struct BintpRequest request[static 1], struct BintpFieldPair *new_field_ptr) {
    struct BintpFieldPair new_field = *new_field_ptr;

    request->field_count += 1;
    int count = request->field_count;
    struct BintpFieldPair *field_list = request->fields;

    field_list = realloc(field_list, sizeof(struct BintpFieldPair[count]));
    field_list[count - 1] = new_field;

    request->fields = field_list;
}

void BintpFreeUpHeader(struct BintpRequest request[static 1]) {
    request->field_count = 0;
    free(request->fields);
}

/*
    Return the inserted content size
 */
static size_t InsertInfoString_(void *dest, char *str, size_t limit) {
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
static size_t InsertHeaderField_(uint8_t *dest, struct BintpFieldPair field) {
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

struct MemPair BintpGenerateRequest(struct BintpRequest *prepare_ptr) {
    struct BintpRequest prepare = *prepare_ptr;

    size_t size = 0;                               // theoretical size
    size += 1;                       // version
    size += sizeof(char[strlen(prepare.uri) + 2]); // URI

    for (int i = 0; i < prepare.field_count; i++) {
        size += 1 + prepare.fields[i].name_size;
        size += 1 + prepare.fields[i].value_size;
    }
    size += 1;

    size += prepare.load_size;

    void *request = malloc(size);

    size_t offset = 0;
    *(uint8_t *)request = prepare.version;
    offset += 1;

    offset += InsertInfoString_(request + offset, prepare.uri, size);

    printf("info end in:\t%zu\n", offset); // TODO debug

    for (int i = 0; i < prepare.field_count; i++)
        offset += InsertHeaderField_(request + offset, prepare.fields[i]);
    offset += InsertHeaderField_(request + offset, HEADER_END_FIELD);

    printf("header end in:\t%zu\n", offset); // TODO debug

    memcpy(request + offset, prepare.load, prepare.load_size);
    if (offset + prepare.load_size != size)
        return MEMPAIR_NULL;

    return (struct MemPair){.size = size, .ptr = request};
}

/*
    Parser
 */

int BintpParseVersion(void *bin, size_t bin_size) {
    return *(uint8_t *)bin;
}

static size_t FindInfoStringRange_(char *str_start, size_t max_size) {
    char *end = strnstr(str_start, "\r\n", max_size); // TODO uri shouldn't have \0 but still...
    if (end == NULL)
        return 0;

    return (end - str_start) + sizeof(char[2]);
}

/*
    Return a standard C string
 */
static char *ParseInfoString_(char *str_start, size_t str_range) {
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
static size_t ParseSingleField_(void *bin, size_t bin_size, struct BintpFieldPair pair[static 1]) {
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

/*
    Just for version 1
 */
size_t BintpParseRequest(void *bin, size_t bin_size, struct BintpRequest form[static 1]) {
    form->version = 1;
    size_t offset = 1;

    size_t uri_range = FindInfoStringRange_(bin + offset, bin_size - offset);
    form->uri = ParseInfoString_(bin + offset, uri_range);
    offset += uri_range;

    form->field_count = 0;
    form->fields = NULL;
    while (true) {
        if (bin_size - offset >= 1 and *(uint8_t *)(bin + offset) == 0) {
            offset += 1;
            break;
        }
        form->field_count += 1;
        form->fields = realloc(form->fields, sizeof(struct BintpFieldPair[form->field_count]));

        offset += ParseSingleField_(bin + offset, bin_size - offset, form->fields + form->field_count - 1);
    }

    return offset;
}
