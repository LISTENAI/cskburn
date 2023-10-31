#pragma once

#include "io.h"

reader_t *filereader_open(const char *filename);

writer_t *filewriter_open(const char *filename);
