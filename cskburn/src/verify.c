#include "verify.h"

#include <stdlib.h>

#include "io.h"
#include "mbedtls/md5.h"

static void
verify_hook(const uint8_t *buf, uint32_t size, void *ctx)
{
	mbedtls_md5_update((mbedtls_md5_context *)ctx, buf, size);
}

void
verify_install_reader(reader_t *reader)
{
	mbedtls_md5_context *ctx = malloc(sizeof(mbedtls_md5_context));
	mbedtls_md5_init(ctx);
	mbedtls_md5_starts(ctx);
	reader_install(reader, verify_hook, (void *)ctx);
}

int
verify_finish_reader(reader_t *reader, uint8_t md5[16])
{
	mbedtls_md5_context *ctx = (mbedtls_md5_context *)reader_hook_ctx(reader);
	int ret = mbedtls_md5_finish(ctx, md5);
	mbedtls_md5_free(ctx);
	free(ctx);
	return ret;
}

void
verify_install_writer(writer_t *writer)
{
	mbedtls_md5_context *ctx = malloc(sizeof(mbedtls_md5_context));
	mbedtls_md5_init(ctx);
	mbedtls_md5_starts(ctx);
	writer_install(writer, verify_hook, (void *)ctx);
}

int
verify_finish_writer(writer_t *writer, uint8_t md5[16])
{
	mbedtls_md5_context *ctx = (mbedtls_md5_context *)writer_hook_ctx(writer);
	int ret = mbedtls_md5_finish(ctx, md5);
	mbedtls_md5_free(ctx);
	free(ctx);
	return ret;
}
