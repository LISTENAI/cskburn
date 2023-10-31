#pragma once

#include <stdint.h>

struct _reader_t;
typedef struct _reader_t reader_t;

typedef void (*reader_hook_t)(const uint8_t *buf, uint32_t size, void *ctx);

struct _reader_t {
	uint32_t (*read)(reader_t *reader, uint8_t *buf, uint32_t size);
	void (*close)(reader_t **reader);
	uint32_t size;
	void *ctx;
	reader_hook_t hook;
	void *hook_ctx;
};

void reader_install(reader_t *reader, reader_hook_t hook, void *ctx);
void *reader_hook_ctx(reader_t *reader);

struct _writer_t;
typedef struct _writer_t writer_t;

struct _writer_t {
	uint32_t (*write)(writer_t *writer, const uint8_t *buf, uint32_t size);
	void (*close)(writer_t **writer);
	void *ctx;
};
