#define _GNU_SOURCE

#include "bintp/bintp1.h"

#include <iso646.h>
#include <stdbool.h>
#include <string.h>

static const uint8_t kReversion = 0xff;

// TODO separate this function
#define HEADER_END_FIELD \
    (struct Bintp1FieldPair) \
    { \
        .name_size = 0, .value_size = 0 \
    }

#define STR_END_MARK_PTR "\0"
#define STR_END_SIZE 1

const uint8_t kBintp1MethodGet = 0xa9;
const uint8_t kBintp1MethodHead = 0xd3;
const uint8_t kBintp1MethodPost = 0x11;
const uint8_t kBintp1MethodPut = 0xa0;
const uint8_t kBintp1MethodDelete = 0x80;
const uint8_t kBintp1MethodConnect = 0x6f;
const uint8_t kBintp1MethodOptions = 0x83;
const uint8_t kBintp1MethodTrace = 0x0d;

/*
    return new tgt_count
 */
int Bintp1AppendField(struct Bintp1Field field[static 1], struct Bintp1FieldPair new_field_ptr[static 1])
{
    struct Bintp1FieldPair new_field = *new_field_ptr;

    int count = field->count;
    count += 1;
    struct Bintp1FieldPair *field_list = field->pairs;

    field_list = realloc(field_list, sizeof(struct Bintp1FieldPair[count]));
    field_list[count - 1] = new_field;

    field->count = count;
    field->pairs = field_list;
    return count;
}

/*
    return the result field index.
    return negative number is error.
 */
int Bintp1SearchField(struct Bintp1Field field[static 1], size_t name_size, void *name_ptr)
{
    for (int i = 0; i < field->count; i++) {
        struct Bintp1FieldPair now_pair = field->pairs[i];
        if (now_pair.name_size == name_size and memcmp(now_pair.name, name_ptr, name_size) == 0)
            return i;
    }

    return -1;
}

/*
    Set a pair in array with a specific name to a new pair.
    If field pair not exist in the array, append it.

    return the index of the changed field.
    return negative number is error.
 */
int Bintp1SetField(struct Bintp1Field field[static 1], struct Bintp1FieldPair new[static 1])
{
    int idx = Bintp1SearchField(field, new->name_size, new->name);
    if (idx < 0)
        idx = Bintp1AppendField(field, new);
    else
        field->pairs[idx] = *new;

    return idx;
}

/*
    The limit of large mode is 32768

    TODO convert to macro
 */
static bool GetFieldPartUseLargeMode_(size_t size)
{
    if (size <= 127)
        return false;
    else
        return true;
}

/*
    Include ending symbol, therefore the return value will >= 1

    return 0: error
 */
static size_t GetFieldsSize_(struct Bintp1Field field[static 1])
{
    size_t size = 0;

    for (int i = 0; i < field->count; i++) {
        struct Bintp1FieldPair temp_pair = field->pairs[i];
        size_t temp_size;

        temp_size = temp_pair.name_size;
        if (temp_size > 32767)
            return 0;
        size += GetFieldPartUseLargeMode_(temp_size) == false ? 1 : 2;
        size += temp_size;

        temp_size = temp_pair.value_size;
        if (temp_size > 32767)
            return 0;
        size += GetFieldPartUseLargeMode_(temp_size) == false ? 1 : 2;
        size += temp_size;
    }
    size += 1;

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

    return 0: error
 */
static size_t InsertSingleField_(void *dest, size_t limit, struct Bintp1FieldPair field)
{
    if (field.name_size == 0)
        return 1;

    size_t offset = 0;
    int ret;

    ret = InsertSingleFieldPart_(dest + offset, limit - offset, field.name, field.name_size); // TODO error check
    if (ret == 0)
        return 0;
    offset += ret;

    ret = InsertSingleFieldPart_(dest + offset, limit - offset, field.value, field.value_size);
    if (ret == 0)
        return 0;
    offset += ret;

    return offset;
}

/*
    return 0: error
 */
static size_t InsertFields_(void *dest, size_t limit, struct Bintp1Field field[static 1])
{
    size_t offset = 0;
    int ret;

    for (int i = 0; i < field->count; i++) {
        ret = InsertSingleField_(dest + offset, limit - offset, field->pairs[i]);
        offset += ret;
    }
    ret = InsertSingleField_(dest + offset, limit - offset, HEADER_END_FIELD);
    if (ret == 0)
        return 0;
    offset += ret;

    return offset;
}

/*
    return 0: error
 */
size_t Bintp1CalcRequestSize(struct Bintp1Request prepare_ptr[static 1])
{
    struct Bintp1Request prepare = *prepare_ptr;
    size_t ret;

    size_t size = 0;                                          // theoretical size
    size += 1 + 1;                                            // version + method
    size += sizeof(char[strlen(prepare.uri) + STR_END_SIZE]); // URI

    ret = GetFieldsSize_(&prepare.field);
    if (ret == 0)
        return 0;
    size += ret;

    return size;
}

/*
    Return the size written

    return 0: error
 */
size_t Bintp1WriteRequest(void *dest, size_t limit, struct Bintp1Request prepare_ptr[static 1])
{
    struct Bintp1Request prepare = *prepare_ptr;
    size_t ret;

    size_t offset = 0;

    // version
    *(uint8_t *)dest = kReversion;
    offset += 1;

    *(uint8_t *)(dest + offset) = prepare.method;
    offset += 1;

    ret = InsertInfoString_(dest + offset, limit, prepare.uri);
    if (ret == 0)
        return 0;
    offset += ret;

    ret = InsertFields_(dest + offset, limit - offset, &prepare.field);
    if (ret == 0)
        return 0;
    offset += ret;

    return offset;
}

void Bintp1FreeUpRequest(struct Bintp1Request form[static 1])
{
    form->field.count = 0;
    free(form->field.pairs);
}

/*
    return 0: error
 */
size_t Bintp1CalcResponseSize(struct Bintp1Response prepare_ptr[static 1])
{
    struct Bintp1Response prepare = *prepare_ptr;
    size_t ret;

    size_t size = 0;
    size += 1 + 2; // version + status code
    ret = GetFieldsSize_(&prepare.field);
    if (ret == 0)
        return 0;
    size += ret;

    return size;
}

/*
    Return the size written

    return 0: error
 */
size_t Bintp1WriteResponse(void *dest, size_t limit, struct Bintp1Response prepare_ptr[static 1])
{
    struct Bintp1Response prepare = *prepare_ptr;
    size_t ret;

    size_t offset = 0;
    *(uint8_t *)(dest + offset) = kReversion;
    offset += 1;

    *(uint16_t *)(dest + offset) = prepare.status;
    offset += 2;

    ret = InsertFields_(dest + offset, limit - offset, &prepare.field);
    if (ret == 0)
        return 0;
    offset += ret;

    return offset;
}

void Bintp1FreeUpResponse(struct Bintp1Response form[static 1])
{
    form->field.count = 0;
    free(form->field.pairs);
}

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

    return NULL: error
 */
static char *ParseInfoString_(char *str_start, size_t str_range)
{
    size_t cstr_size = str_range - STR_END_SIZE + 1;
    char *str_buffer = malloc(sizeof(char[cstr_size]));
    if (str_buffer == NULL)
        return NULL;

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
 */
static size_t ParseSingleField_(void *bin, size_t limit, struct Bintp1FieldPair pair[static 1])
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
 */
static size_t ParseFields_(void *bin, size_t bin_size, struct Bintp1Field field[static 1])
{
    if (bin_size <= 0)
        return 0;

    int ret;

    size_t offset = 0;
    int count = 0;
    struct Bintp1FieldPair *list_ptr = NULL;

    while (true) {
        if (*(uint8_t *)(bin + offset) == 0) {
            offset += 1;
            break;
        }
        count += 1;
        list_ptr = realloc(list_ptr, sizeof(struct Bintp1FieldPair[count]));

        ret = ParseSingleField_(bin + offset, bin_size - offset, list_ptr + count - 1);
        if (ret == 0)
            return 0;
        offset += ret;
    }

    field->count = count;
    field->pairs = list_ptr;
    return offset;
}

/*
    Just for version 1

    Parses only the header, not the load

    Return 0: error
 */
size_t Bintp1ParseRequest(void *bin, size_t bin_size, struct Bintp1Request form[static 1])
{
    size_t ret;
    size_t offset = 1; // skip version

    form->method = *(uint8_t *)(bin + offset);
    offset += 1;

    ret = FindInfoStringRange_(bin + offset, bin_size - offset);
    if (ret == 0)
        return 0;
    size_t uri_range = ret;
    form->uri = ParseInfoString_(bin + offset, uri_range);
    if (form->uri == NULL)
        return 0;
    offset += uri_range;

    ret = ParseFields_(bin + offset, bin_size - offset, &form->field);
    if (ret == 0)
        return 0;
    offset += ret;

    return offset;
}

/*
    Parse only the header, and return thats size

    return 0: error
 */
size_t Bintp1ParseResponse(void *bin, size_t bin_size, struct Bintp1Response form[static 1])
{
    size_t ret;
    size_t offset = 1; // skip version

    form->status = *(uint8_t *)(bin + offset);
    offset += 1;

    ret = ParseFields_(bin + offset, bin_size - offset, &form->field);
    if (ret == 0)
        return 0;
    offset += ret;

    return offset;
}
