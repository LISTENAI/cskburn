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
verify_install(reader_t *reader)
{
	mbedtls_md5_context *ctx = malloc(sizeof(mbedtls_md5_context));
	mbedtls_md5_init(ctx);
	mbedtls_md5_starts(ctx);
	reader_install(reader, verify_hook, (void *)ctx);
}

int
verify_finish(reader_t *reader, uint8_t md5[16])
{
	mbedtls_md5_context *ctx = (mbedtls_md5_context *)reader_hook_ctx(reader);
	int ret = mbedtls_md5_finish(ctx, md5);
	mbedtls_md5_free(ctx);
	free(ctx);
	return ret;
}
