#ifndef BINTP_TOOL_DUMP_H_
#define BINTP_TOOL_DUMP_H_

#include <stdlib.h>

#include "bintp/bintp1.h"

void DumpHex_(void *ptr, size_t size);
void DumpBintpField_(struct Bintp1FieldPair *field);

#endif
