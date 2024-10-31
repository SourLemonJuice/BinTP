#include <iso646.h>
#include <stdbool.h>
#include <string.h>

#include "memory.h"

/*
    Check whether MemPair is NULL
 */
bool MemPairIsNull(struct MemPair *pair) {
    if (memcmp(pair, &MEMPAIR_NULL, sizeof(struct MemPair)) == 0)
        return true;
    return false;
}

/*
    Search "needle" in "haystack", limited to the first "len" chars of haystack
 */
char *strnstr(char haystack[const restrict static 1], char needle[const restrict static 1], int len) {
    if (len <= 0)
        return NULL;

    char *temp_str;
    int needle_len = strlen(needle);
    while (true) {
        temp_str = strchr(haystack, needle[0]);
        if (temp_str == NULL)
            return NULL;

        if ((temp_str - haystack) + needle_len > len)
            return NULL;
        if (strncmp(temp_str, needle, needle_len) == 0)
            return temp_str;
    }
}
