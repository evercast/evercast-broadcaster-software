#pragma once

#include <inttypes.h>

#include "darray.h"

void cl_init();
void cl_free();
int cl_assign_label(int code, char *label);
void cl_record(int index, uint64_t flags);
