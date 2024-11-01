#ifndef BINTP_MEMORY_H_
#define BINTP_MEMORY_H_

#include <stdbool.h>
#include <stdlib.h>

struct MemPair {
    size_t size;
    void *ptr;
};

#define MEMPAIR_NULL \
    (struct MemPair) { \
        .size = 0, .ptr = NULL \
    }

bool MemPairIsNull(struct MemPair *pair);
char *strnstr(char haystack[const restrict static 1], char needle[const restrict static 1], int len);

#endif
