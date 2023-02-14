#include "io.h"

void
reader_install(reader_t *reader, reader_hook_t hook, void *ctx)
{
	reader->hook = hook;
	reader->hook_ctx = ctx;
}

void *
reader_hook_ctx(reader_t *reader)
{
	return reader->hook_ctx;
}
