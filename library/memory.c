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
