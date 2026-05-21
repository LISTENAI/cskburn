#pragma once

#include <stdint.h>

#include "io.h"

void verify_install_reader(reader_t *reader);
int verify_finish_reader(reader_t *reader, uint8_t md5[16]);

void verify_install_writer(writer_t *writer);
int verify_finish_writer(writer_t *writer, uint8_t md5[16]);
