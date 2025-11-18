#pragma once

#include "io.h"

reader_t *memreader_alloc(uint32_t size);
uint32_t memreader_feed(reader_t *reader, const uint8_t *buf, uint32_t size);
uint32_t memreader_fill(reader_t *reader, uint8_t value, uint32_t size);
