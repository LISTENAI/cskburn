#include "fsio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	FILE *fp;
} filereader_ctx_t;

uint32_t filereader_read(reader_t *reader, uint8_t *buf, uint32_t size);
void filereader_close(reader_t **reader);

reader_t *
filereader_open(const char *filename)
{
	FILE *fp = fopen(filename, "rb");
	if (fp == NULL) {
		return NULL;
	}

	filereader_ctx_t *ctx = calloc(1, sizeof(filereader_ctx_t));
	ctx->fp = fp;

	reader_t *reader = calloc(1, sizeof(reader_t));
	reader->read = filereader_read;
	reader->close = filereader_close;
	reader->ctx = ctx;

	fseek(fp, 0, SEEK_END);
	reader->size = (uint32_t)ftell(fp);
	fseek(fp, 0, SEEK_SET);

	return reader;
}

uint32_t
filereader_read(reader_t *reader, uint8_t *buf, uint32_t size)
{
	filereader_ctx_t *ctx = (filereader_ctx_t *)reader->ctx;
	uint32_t bytes = fread(buf, 1, size, ctx->fp);
	if (reader->hook) {
		reader->hook((const uint8_t *)buf, bytes, reader->hook_ctx);
	}
	return bytes;
}

void
filereader_close(reader_t **reader)
{
	filereader_ctx_t *ctx = (filereader_ctx_t *)(*reader)->ctx;
	fclose(ctx->fp);
	free(ctx);
	free(*reader);
	*reader = NULL;
}

typedef struct {
	FILE *fp;
} filewriter_ctx_t;

uint32_t filewriter_write(writer_t *writer, const uint8_t *buf, uint32_t size);
void filewriter_close(writer_t **writer);

writer_t *
filewriter_open(const char *filename)
{
	FILE *fp = fopen(filename, "wb+");
	if (fp == NULL) {
		return NULL;
	}

	filewriter_ctx_t *ctx = calloc(1, sizeof(filewriter_ctx_t));
	ctx->fp = fp;

	writer_t *writer = calloc(1, sizeof(writer_t));
	writer->write = filewriter_write;
	writer->close = filewriter_close;
	writer->ctx = ctx;

	return writer;
}

uint32_t
filewriter_write(writer_t *writer, const uint8_t *buf, uint32_t size)
{
	filewriter_ctx_t *ctx = (filewriter_ctx_t *)writer->ctx;
	uint32_t bytes = fwrite(buf, 1, size, ctx->fp);
	return bytes;
}

void
filewriter_close(writer_t **writer)
{
	filewriter_ctx_t *ctx = (filewriter_ctx_t *)(*writer)->ctx;
	fclose(ctx->fp);
	free(ctx);
	free(*writer);
	*writer = NULL;
}
