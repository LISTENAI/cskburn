#include "memio.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
	uint8_t *buffer;
	uint32_t capacity;
	uint32_t feed_off;
	uint32_t read_off;
} memreader_ctx_t;

uint32_t mem_read(reader_t *reader, uint8_t *buf, uint32_t size);
void mem_close(reader_t **reader);

reader_t *
mem_alloc(uint32_t size)
{
	uint8_t *buffer = malloc(size);
	if (buffer == NULL) {
		return NULL;
	}

	memreader_ctx_t *ctx = malloc(sizeof(memreader_ctx_t));
	ctx->buffer = buffer;
	ctx->capacity = size;
	ctx->feed_off = 0;
	ctx->read_off = 0;

	reader_t *reader = malloc(sizeof(reader_t));
	reader->read = mem_read;
	reader->close = mem_close;
	reader->ctx = ctx;
	reader->size = 0;

	return reader;
}

uint32_t
mem_feed(reader_t *reader, const uint8_t *buf, uint32_t size)
{
	memreader_ctx_t *ctx = (memreader_ctx_t *)reader->ctx;
	if (ctx->feed_off >= ctx->capacity) {
		return 0;
	}
	if (ctx->feed_off + size > ctx->capacity) {
		size = ctx->capacity - ctx->feed_off;
	}
	memcpy(ctx->buffer + ctx->feed_off, buf, size);
	ctx->feed_off += size;
	reader->size = ctx->feed_off;
	return size;
}

uint32_t
mem_read(reader_t *reader, uint8_t *buf, uint32_t size)
{
	memreader_ctx_t *ctx = (memreader_ctx_t *)reader->ctx;
	if (ctx->read_off + size > reader->size) {
		size = reader->size - ctx->read_off;
	}
	memcpy(buf, ctx->buffer + ctx->read_off, size);
	ctx->read_off += size;
	if (reader->hook) {
		reader->hook((const uint8_t *)buf, size, reader->hook_ctx);
	}
	return size;
}

void
mem_close(reader_t **reader)
{
	memreader_ctx_t *ctx = (memreader_ctx_t *)(*reader)->ctx;
	free(ctx->buffer);
	free(ctx);
	free(*reader);
	*reader = NULL;
}
