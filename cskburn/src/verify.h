#pragma once

#include <stdint.h>

#include "io.h"

void verify_install(reader_t *reader);
int verify_finish(reader_t *reader, uint8_t md5[16]);
