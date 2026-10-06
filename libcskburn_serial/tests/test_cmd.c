/* 使用真实命令实现与模拟 SLIP 应答，覆盖设备协议的异常分支。 */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "cmd.h"
#include "core.h"

static uint8_t frames[8][8192];
static ssize_t sizes[8];
static unsigned count, next, writes;
static uint8_t sent[8192];
static size_t sent_size;
static uint8_t output[8192];
static uint32_t output_size;

void serial_discard_input(serial_dev_t *dev) { (void)dev; }
void serial_discard_output(serial_dev_t *dev) { (void)dev; }
int serial_set_speed(serial_dev_t *dev, uint32_t speed) { (void)dev; (void)speed; return 0; }
ssize_t slip_write(slip_dev_t *dev, const uint8_t *buf, size_t len, uint64_t timeout)
{
    (void)dev; (void)timeout;
    assert(len <= sizeof(sent));
    memcpy(sent, buf, len); sent_size = len; writes++;
    return len;
}
ssize_t slip_read(slip_dev_t *dev, uint8_t *buf, size_t len, uint64_t timeout)
{
    (void)dev; (void)timeout;
    if (next == count) return -ETIMEDOUT;
    ssize_t n = sizes[next];
    if (n >= 0) {
        if ((size_t)n > len) return -ENOMEM;
        memcpy(buf, frames[next], n);
    }
    next++;
    return n;
}
/* core.c 链接所需接口；本测试只调用已连接设备的 API。 */
int serial_open(const char *path, serial_dev_t **dev) { (void)path; (void)dev; return -EIO; }
void serial_close(serial_dev_t **dev) { (void)dev; }
int serial_set_rts(serial_dev_t *dev, bool active) { (void)dev; (void)active; return 0; }
int serial_set_dtr(serial_dev_t *dev, bool active) { (void)dev; (void)active; return 0; }
ssize_t serial_read(serial_dev_t *dev, void *buf, size_t len, uint64_t timeout)
{ (void)dev; (void)buf; (void)len; (void)timeout; return -EIO; }
ssize_t serial_write(serial_dev_t *dev, const void *buf, size_t len, uint64_t timeout)
{ (void)dev; (void)buf; (void)len; (void)timeout; return -EIO; }
slip_dev_t *slip_init(serial_dev_t *dev, size_t tx, size_t rx)
{ (void)dev; (void)tx; (void)rx; return NULL; }
void slip_deinit(slip_dev_t **dev) { (void)dev; }
static void reset(void) { count = next = writes = output_size = 0; }
static void raw(const void *data, size_t len)
{
    assert(count < 8 && len <= sizeof(frames[0]));
    memcpy(frames[count], data, len); sizes[count++] = len;
}
static void response(uint8_t op, uint8_t first, uint8_t second, const void *payload, size_t len)
{
    uint8_t buf[8192] = {1, op};
    uint16_t n = len + 2;
    memcpy(buf + 2, &n, 2);
    buf[8] = first; buf[9] = second;
    if (len) memcpy(buf + 10, payload, len);
    raw(buf, len + 10);
}
static uint32_t write_output(writer_t *writer, const uint8_t *data, uint32_t len)
{
    (void)writer;
    assert(output_size + len <= sizeof(output));
    memcpy(output + output_size, data, len); output_size += len;
    return len;
}
int main(void)
{
    uint8_t req[MAX_REQ_RAW_LEN] = {0}, res[MAX_RES_RAW_LEN] = {0}, data[4096] = {0}, md5[16];
    cskburn_serial_device_t dev = {.req_buf=req, .res_buf=res, .req_hdr=req,
        .req_cmd=req+sizeof(csk_command_t), .chip=CHIP_ARCS, .loader_running=true};
    writer_t writer = {.write=write_output};
    uint32_t n;
    emmc_info_t info;
    cskburn_flash_layout_t layout;
    /* ARCS 忙错误必须保留 0x0A，不能变成通用状态 0x01。 */
    reset(); response(0x43, 0x0A, 1, NULL, 0);
    assert(cmd_emmc_block(&dev, data, 1, 0) == 0x0A);
    reset(); response(0x41, 0x05, 1, NULL, 0);
    assert(cmd_emmc_get_info(&dev, &info) == 0x05);
    reset(); response(0x45, 0x06, 1, NULL, 0);
    assert(cmd_emmc_md5(&dev, 0, 1, md5) == 0x06);
    reset(); response(0xF6, 0xFF, 1, NULL, 0);
    assert(cmd_get_flash_layout(&dev, &layout) == 0xFF);
    reset(); response(0xD2, 0x05, 1, NULL, 0);
    assert(cmd_read_flash_stream(&dev, 0, 1, &writer, md5, NULL) == 0x05);
    /* VenusA 与 ROM 保持 status/error 顺序。 */
    dev.chip = CHIP_VENUSA;
    reset(); response(0xD5, 1, 0xC4, NULL, 0);
    assert(cmd_flash_unlock(&dev) == 0xC4);
    dev.chip = CHIP_ARCS; dev.loader_running = false;
    reset(); response(0x07, 1, 0x07, NULL, 0);
    assert(cmd_mem_block(&dev, data, 1, 0) == 0x07);
    reset(); response(0x07, 1, 0, NULL, 0);
    assert(cmd_mem_block(&dev, data, 1, 0) == -EIO);
    dev.loader_running = true;
    /* 截断状态、短 MD5、短读、超长读均不能报告成功。 */
    reset(); response(0xD5, 0, 0, NULL, 0); sizes[0]--; frames[0][2] = 1;
    assert(cmd_flash_unlock(&dev) == -EIO);
    reset(); response(0x45, 0, 0, data, 15);
    assert(cmd_emmc_md5(&dev, 0, 1, md5) == -EIO);
    reset(); response(0x46, 0, 0, data, 7);
    assert(cmd_read_emmc(&dev, 0, 8, data, &n) == -EIO);
    reset(); response(0x46, 0, 0, data, 9);
    assert(cmd_read_emmc(&dev, 0, 8, data, &n) == -EIO);
    uint8_t oversized[4097] = {0};
    reset(); response(0x46, 0, 0, oversized, sizeof(oversized));
    assert(cmd_read_emmc(&dev, 0, 4096, data, &n) == -ENOMEM);
    reset(); response(0x0E, 0, 0, oversized, 65);
    assert(cmd_read_flash(&dev, 0, 64, data, &n) == -EIO);
    reset(); response(0x46, 0, 0, data, 8); sizes[0]--;
    assert(cmd_read_emmc(&dev, 0, 8, data, &n) == -EIO);
    reset(); response(0x46, 0, 0, data, 8);
    assert(cmd_read_emmc(&dev, 0, 8, data, &n) == 0 && n == 8);
    reset();
    assert(cmd_read_emmc(&dev, 0, 4097, data, &n) == -EINVAL && writes == 0);
    assert(cmd_read_flash(&dev, 0, 65, data, &n) == -EINVAL && writes == 0);
    /* 分块读取含短末块、累计 ACK、末尾 MD5，即使调用方未请求校验也消费 MD5。 */
    memset(data, 0xC0, sizeof(data)); memset(md5, 0xDB, sizeof(md5));
    reset(); response(0xD2, 0, 0, NULL, 0); raw(data, 4096); raw(data, 3); raw(md5, 16);
    uint8_t got_md5[16];
    assert(cmd_read_flash_stream(&dev, 0, 4099, &writer, got_md5, NULL) == 0);
    assert(output_size == 4099 && writes == 3 && next == count);
    memcpy(&n, sent, 4); assert(sent_size == 4 && n == 4099);
    assert(memcmp(got_md5, md5, 16) == 0 && output[4098] == 0xC0);
    reset(); response(0xD2, 0, 0, NULL, 0); raw(data, 1); raw(md5, 16);
    assert(cmd_read_flash_stream(&dev, 0, 1, &writer, NULL, NULL) == 0 && next == count);
    reset(); response(0xD2, 0, 0, NULL, 0); raw(data, 1);
    assert(cmd_read_flash_stream(&dev, 0, 1, &writer, md5, NULL) == -ETIMEDOUT);
    reset(); response(0xD2, 0, 0, NULL, 0); raw(data, 1); raw(md5, 15);
    assert(cmd_read_flash_stream(&dev, 0, 1, &writer, md5, NULL) == -EIO);
    reset(); response(0xD2, 0, 0, NULL, 0); raw(data, 2);
    assert(cmd_read_flash_stream(&dev, 0, 1, &writer, md5, NULL) == -EIO);
    /* 能力协商只影响当前设备；旧 Loader 保持普通读取。 */
    const struct cskburn_serial_burner_info burner_info = {.supports_flash_layout=true,
        .supports_flash_lock=true};
    dev.burner_info = &burner_info;
    cskburn_serial_device_t other = dev;
    layout = (cskburn_flash_layout_t){.version=1, .capabilities=3, .flash_count=1,
        .total_size=0x1000000, .flash_size={0x1000000,0}};
    reset(); response(0xF6, 0, 0, &layout, sizeof(layout));
    assert(cskburn_serial_get_flash_layout(&dev, &layout) == 0 && dev.read_stream);
    assert(!other.read_stream && !other.flash_layout_queried);
    reset(); response(0xD2, 0, 0, NULL, 0); raw(data, 1); raw(md5, 16);
    assert(cskburn_serial_read(&dev, TARGET_FLASH, 0, 1, &writer, NULL, NULL) == 0);
    assert(writes == 2 && next == count);
    layout.capabilities = 1;
    reset(); response(0xF6, 0, 0, &layout, sizeof(layout));
    assert(cskburn_serial_get_flash_layout(&dev, &layout) == 0 && !dev.read_stream);
    reset(); response(0x0E, 0, 0, data, 1);
    assert(cskburn_serial_read(&dev, TARGET_FLASH, 0, 1, &writer, NULL, NULL) == 0);
    assert(sent[1] == 0x0E);
    dev.flash_layout_queried = false;
    reset(); response(0xF6, 0xFF, 1, NULL, 0); response(0x0E, 0, 0, data, 1);
    assert(cskburn_serial_read(&dev, TARGET_FLASH, 0, 1, &writer, NULL, NULL) == 0);
    layout.total_size++;
    reset(); response(0xF6, 0, 0, &layout, sizeof(layout));
    assert(cskburn_serial_get_flash_layout(&dev, &layout) == -EIO && !dev.read_stream);
    cskburn_flash_protection_t protection;
    reset(); response(0xD6, 0xFF, 1, NULL, 0);
    assert(cskburn_serial_get_flash_protection(&dev, TARGET_FLASH, &protection) == -ENOTSUP);
    reset(); response(0xD6, 0xC4, 1, NULL, 0);
    assert(cskburn_serial_get_flash_protection(&dev, TARGET_FLASH, &protection) == 0xC4);
    reset();
    assert(cskburn_serial_erase(&dev, TARGET_EMMC, 0, 0) == -EINVAL && writes == 0);
    assert(cskburn_serial_erase(&dev, TARGET_EMMC, 0xFFFFFFFE, 3) == -EINVAL && writes == 0);
    puts("PASS: Loader status order, short/error responses, read lengths, stream ACK and final MD5");
    return 0;
}
