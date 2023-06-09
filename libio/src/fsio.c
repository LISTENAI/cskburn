#include "fsio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	FILE *fp;
} filereader_ctx_t;

uint32_t file_read(reader_t *reader, uint8_t *buf, uint32_t size);
void file_close(reader_t **reader);

reader_t *
file_open(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		return NULL;
	}

	filereader_ctx_t *ctx = malloc(sizeof(filereader_ctx_t));
	memset(ctx, 0, sizeof(filereader_ctx_t));
	ctx->fp = fp;

	reader_t *reader = malloc(sizeof(reader_t));
	memset(reader, 0, sizeof(reader_t));
	reader->read = file_read;
	reader->close = file_close;
	reader->ctx = ctx;

	fseek(fp, 0, SEEK_END);
	reader->size = (uint32_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	return reader;
}

uint32_t
file_read(reader_t *reader, uint8_t *buf, uint32_t size)
{
	filereader_ctx_t *ctx = (filereader_ctx_t *)reader->ctx;
	uint32_t bytes = fread(buf, 1, size, ctx->fp);
	if (reader->hook) {
		reader->hook((const uint8_t *)buf, bytes, reader->hook_ctx);
	}
	return bytes;
}

void
file_close(reader_t **reader)
{
	filereader_ctx_t *ctx = (filereader_ctx_t *)(*reader)->ctx;
	fclose(ctx->fp);
	free(ctx);
	free(*reader);
	*reader = NULL;
}
