#pragma once

#include "io.h"

reader_t *mem_alloc(uint32_t size);
uint32_t mem_feed(reader_t *reader, const uint8_t *buf, uint32_t size);
