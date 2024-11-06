#define _GNU_SOURCE

#include "bintp1.h"

#include <iso646.h>
#include <stdio.h> // TODO just for debug
#include <string.h>

#define HEADER_END_FIELD \
    (struct BintpFieldPair) \
    { \
        .name_size = 0, .value_size = 0 \
    }

#define STR_END_MARK_PTR "\0"
#define STR_END_SIZE 1

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

/*
    The limit of large mode is 32768
 */
static bool GetFieldPartUseLargeMode_(size_t size)
{
    if (size <= 127)
        return false;
    else
        return true;
}

static size_t GetFieldsSize_(int count, struct BintpFieldPair fields[static count])
{
    size_t size = 0;

    for (int i = 0; i < count; i++) {
        size_t temp_size = fields[i].name_size;
        size += GetFieldPartUseLargeMode_(temp_size) == false ? 1 : 2;
        size += temp_size;

        temp_size = fields[i].value_size;
        size += GetFieldPartUseLargeMode_(temp_size) == false ? 1 : 2;
        size += temp_size;
    }

    return size;
}

/*
    Return the inserted content size

    return 0: error
 */
static size_t InsertInfoString_(void *dest, size_t limit, char *str)
{
    int str_len = strlen(str);
    size_t str_size = sizeof(char[str_len]);

    if (str_size + STR_END_SIZE > limit)
        return 0;

    memcpy(dest, str, str_size);
    memcpy(dest + str_size, STR_END_MARK_PTR, STR_END_SIZE);

    return str_size + STR_END_SIZE;
}

/*
    return 0: error
 */
static size_t InsertSingleFieldPart_(void *dest, size_t dest_limit, void *src_ptr, size_t src_size)
{
    // dest must have 2 byte, at that case: size(1byte) + load(>= 1byte)
    if (src_size <= 0 or dest_limit < 2)
        return 0;

    size_t offset = 0;

    // use 8bit or 16bit
    if (GetFieldPartUseLargeMode_(src_size) == false) {
        *(uint8_t *)dest = (src_size << 1) | 0;
        offset += 1;
    } else {
        *(uint16_t *)dest = (src_size << 1) | 1;
        offset += 2;
    }

    if (offset + src_size > dest_limit)
        return 0;

    memcpy(dest + offset, src_ptr, src_size);
    offset += src_size;

    return offset;
}

/*
    If field.name_size == 0, then just insert one byte with zero.
 */
static size_t InsertSingleField_(void *dest, size_t limit, struct BintpFieldPair field)
{
    if (field.name_size == 0)
        return 1;

    size_t offset = 0;

    offset += InsertSingleFieldPart_(dest + offset, limit - offset, field.name, field.name_size); // TODO error check
    offset += InsertSingleFieldPart_(dest + offset, limit - offset, field.value, field.value_size);

    return offset;
}

/*
    TODO error check
 */
static size_t InsertFields_(void *dest, size_t limit, int count, struct BintpFieldPair fields[static count])
{
    size_t offset = 0;

    for (int i = 0; i < count; i++)
        offset += InsertSingleField_(dest + offset, limit - offset, fields[i]);
    offset += InsertSingleField_(dest + offset, limit - offset, HEADER_END_FIELD);

    return offset;
}

/*
    return MEMPAIR_NULL: error
 */
struct MemPair BintpGenerateRequest(struct BintpRequest *prepare_ptr)
{
    struct BintpRequest prepare = *prepare_ptr;

    size_t size = 0;                                          // theoretical size
    size += 1 + 1;                                            // version + method
    size += sizeof(char[strlen(prepare.uri) + STR_END_SIZE]); // URI

    size += GetFieldsSize_(prepare.field_count, prepare.fields);
    size += 1;

    void *request = malloc(size);

    printf("size:\t%zu\n", size);

    size_t offset = 0;
    *(uint8_t *)request = prepare.version;
    offset += 1;

    *(uint8_t *)(request + offset) = prepare.method;
    offset += 1;

    offset += InsertInfoString_(request + offset, size, prepare.uri);

    printf("info end in:\t%zu\n", offset); // TODO debug

    offset += InsertFields_(request + offset, size - offset, prepare.field_count, prepare.fields);

    printf("header end in:\t%zu\n", offset); // TODO debug

    // TODO half debug
    if (offset != size)
        return MEMPAIR_NULL;

    return (struct MemPair){.size = size, .ptr = request};
}

struct MemPair BintpGenerateResponse(struct BintpResponse prepare_ptr[static 1])
{
    struct BintpResponse prepare = *prepare_ptr;

    size_t size = 0;
    size += 1 + 2; // version + status code
    size += GetFieldsSize_(prepare.field_count, prepare.fields);

    void *response = malloc(size);

    size_t offset = 0;
    *(uint8_t *)(response + offset) = prepare.version;
    offset += 1;

    *(uint8_t *)(response + offset) = prepare.status;
    offset += 1;

    offset += InsertFields_(response + offset, size - offset, prepare.field_count, prepare.fields);

    if (offset != size)
        return MEMPAIR_NULL;

    return (struct MemPair){.size = size, .ptr = response};
}

/*
    Parser
 */

/*
    return -1: bin_size invalid
 */
int BintpParseVersion(void *bin, size_t bin_size)
{
    if (bin_size < 1)
        return -1;

    return *(uint8_t *)bin;
}

/*
    return 0: not found
 */
static size_t FindInfoStringRange_(char *info_start, size_t limit)
{
    char *end = memmem(info_start, limit, STR_END_MARK_PTR, STR_END_SIZE);
    if (end == NULL)
        return 0;

    return (end - info_start) + STR_END_SIZE;
}

/*
    Return a standard C string
 */
static char *ParseInfoString_(char *str_start, size_t str_range)
{
    size_t cstr_size = str_range - STR_END_SIZE + 1;
    char *str_buffer = malloc(sizeof(char[cstr_size]));
    if (str_buffer == NULL)
        return 0;

    memcpy(str_buffer, str_start, cstr_size);
    memcpy(str_buffer + cstr_size, STR_END_MARK_PTR, STR_END_SIZE);

    return str_buffer;
}

/*
    Return the offset of the content relative to bin.
    The size of content will be write into length.

    Return 0: invalid parameter
 */
static size_t ParseSingleFieldPartSize_(void *bin, size_t limit, size_t length[static 1])
{
    if (limit <= 0)
        return 0;

    size_t offset;
    size_t name_size;
    if ((*(uint8_t *)bin & 1) != 0) {
        // 16bit mode
        name_size = *(uint16_t *)bin >> 1;
        offset = 2;
    } else {
        // 8bit mode
        name_size = *(uint8_t *)bin >> 1;
        offset = 1;
    }
    *length = name_size;

    return offset;
}

/*
    if bin_size is 0, need throw an error(return 0)

    TODO more error check
 */
static size_t ParseSingleField_(void *bin, size_t limit, struct BintpFieldPair pair[static 1])
{
    if (limit <= 0)
        return 0;

    size_t offset = 0;

    size_t name_size;
    size_t name_front = ParseSingleFieldPartSize_(bin + offset, limit - offset, &name_size);
    if (name_front == 0)
        return 0;
    offset += name_front;

    void *name = malloc(name_size);
    memcpy(name, bin + offset, name_size);
    pair->name = name;
    pair->name_size = name_size;
    offset += name_size;

    size_t value_size;
    size_t value_front = ParseSingleFieldPartSize_(bin + offset, limit - offset, &value_size);
    if (value_front == 0)
        return 0;
    offset += value_front;

    void *value = malloc(value_size);
    memcpy(value, bin + offset, value_size);
    pair->value = value;
    pair->value_size = value_size;
    offset += value_size;

    return offset;
}

/*
    return 0: error occurs

    TODO more error check
 */
static size_t ParseFields_(void *bin, size_t bin_size, int tgt_count[static 1], struct BintpFieldPair **tgt_list)
{
    if (bin_size <= 0)
        return 0;

    size_t offset = 0;
    int count = 0;
    struct BintpFieldPair *list_ptr = NULL;

    while (true) {
        if (*(uint8_t *)(bin + offset) == 0) {
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

    Return 0: content invalid
 */
size_t BintpParseRequest(void *bin, size_t bin_size, struct BintpRequest form[static 1])
{
    form->version = 1;
    size_t offset = 1;

    form->method = *(uint8_t *)(bin + offset);
    offset += 1;

    size_t uri_range = FindInfoStringRange_(bin + offset, bin_size - offset);
    if (uri_range == 0)
        return 0;
    form->uri = ParseInfoString_(bin + offset, uri_range);
    offset += uri_range;

    offset += ParseFields_(bin + offset, bin_size - offset, &form->field_count, &form->fields);

    return offset;
}

size_t BintpParseResponse(void *bin, size_t bin_size, struct BintpResponse form[static 1])
{
    form->version = 1;
    size_t offset = 1;

    form->status = *(uint8_t *)(bin + offset);
    offset += 1;

    offset += ParseFields_(bin + offset, bin_size - offset, &form->field_count, &form->fields);

    return offset;
}
