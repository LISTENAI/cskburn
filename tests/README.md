# cskburn 测试套件

本目录包含 cskburn 项目的单元测试和集成测试。

## 测试框架

推荐使用以下测试框架之一：

### 1. Unity (推荐 - 轻量级)
- 网站: http://www.throwtheswitch.org/unity
- 特点: 轻量级，专为嵌入式和 C 项目设计
- 安装: 作为 git submodule 添加到 `third/unity`

```cmake
# CMakeLists.txt 示例
add_subdirectory(third/unity)
add_executable(test_slip test_slip.c)
target_link_libraries(test_slip slip unity)
add_test(NAME test_slip COMMAND test_slip)
```

### 2. cmocka (推荐 - 带 Mock 支持)
- 网站: https://cmocka.org/
- 特点: 支持 mock，测试隔离好
- 安装: 通过包管理器或编译安装

```cmake
# CMakeLists.txt 示例
find_package(cmocka REQUIRED)
add_executable(test_serial test_serial.c)
target_link_libraries(test_serial serial ${CMOCKA_LIBRARIES})
add_test(NAME test_serial COMMAND test_serial)
```

## 测试结构

```
tests/
├── CMakeLists.txt          # 测试构建配置
├── README.md               # 本文件
├── unit/                   # 单元测试
│   ├── test_slip.c         # SLIP 协议测试
│   ├── test_serial.c       # 串口通信测试
│   └── test_cskburn_cmd.c  # 命令构建测试
├── integration/            # 集成测试
│   ├── test_serial_burn.c  # 串口烧录集成测试
│   └── test_usb_burn.c     # USB 烧录集成测试
└── fixtures/               # 测试数据
    ├── test_firmware.bin
    └── test_firmware.hex
```

## 待添加的测试

### 高优先级
- [ ] `test_slip.c` - SLIP 编码/解码测试
- [ ] `test_intelhex.c` - Intel HEX 解析测试
- [ ] `test_cmd.c` - 命令构建测试

### 中优先级
- [ ] `test_serial.c` - 串口通信测试 (需要 mock)
- [ ] `test_verify.c` - 数据验证测试
- [ ] `test_utils.c` - 工具函数测试

### 低优先级
- [ ] 集成测试 (需要硬件或模拟器)
- [ ] 性能测试
- [ ] 压力测试

## 示例测试文件

### test_slip.c 示例 (使用 Unity)

```c
#include "unity.h"
#include "slip.h"

void setUp(void) {
    // 每个测试前的设置
}

void tearDown(void) {
    // 每个测试后的清理
}

void test_slip_encode_basic(void) {
    uint8_t input[] = {0x01, 0x02, 0x03};
    uint8_t output[10];
    size_t output_len;
    
    int ret = slip_encode(input, sizeof(input), output, sizeof(output), &output_len);
    
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(5, output_len); // START + 3 bytes + END
    TEST_ASSERT_EQUAL(SLIP_START, output[0]);
    TEST_ASSERT_EQUAL(SLIP_END, output[4]);
}

void test_slip_encode_escape(void) {
    // 测试需要转义的字符
    uint8_t input[] = {SLIP_END, SLIP_ESC};
    uint8_t output[10];
    size_t output_len;
    
    int ret = slip_encode(input, sizeof(input), output, sizeof(output), &output_len);
    
    TEST_ASSERT_EQUAL(0, ret);
    // START + ESC+ESC_END + ESC+ESC_ESC + END = 7 bytes
    TEST_ASSERT_EQUAL(7, output_len);
}

void test_slip_decode_basic(void) {
    uint8_t input[] = {SLIP_START, 0x01, 0x02, 0x03, SLIP_END};
    uint8_t output[10];
    size_t output_len;
    
    int ret = slip_decode(input, sizeof(input), output, sizeof(output), &output_len);
    
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(3, output_len);
    TEST_ASSERT_EQUAL(0x01, output[0]);
    TEST_ASSERT_EQUAL(0x02, output[1]);
    TEST_ASSERT_EQUAL(0x03, output[2]);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_slip_encode_basic);
    RUN_TEST(test_slip_encode_escape);
    RUN_TEST(test_slip_decode_basic);
    
    return UNITY_END();
}
```

### test_intelhex.c 示例 (使用 cmocka)

```c
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "intelhex.h"

static void test_parse_data_record(void **state) {
    const char *line = ":10010000214601360121470136007EFE09D2194097";
    uint8_t data[16];
    uint32_t addr;
    size_t len;
    
    int ret = intelhex_parse_line(line, &addr, data, &len);
    
    assert_int_equal(ret, 0);
    assert_int_equal(addr, 0x0100);
    assert_int_equal(len, 16);
    assert_int_equal(data[0], 0x21);
}

static void test_parse_eof_record(void **state) {
    const char *line = ":00000001FF";
    
    int ret = intelhex_parse_line(line, NULL, NULL, NULL);
    
    assert_int_equal(ret, INTELHEX_EOF);
}

static void test_parse_invalid_checksum(void **state) {
    const char *line = ":10010000214601360121470136007EFE09D2194000";
    
    int ret = intelhex_parse_line(line, NULL, NULL, NULL);
    
    assert_int_equal(ret, INTELHEX_ERR_CHECKSUM);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_parse_data_record),
        cmocka_unit_test(test_parse_eof_record),
        cmocka_unit_test(test_parse_invalid_checksum),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}
```

## 运行测试

### 构建测试
```bash
# 配置构建 (启用测试)
cmake -B build -G Ninja -DENABLE_TESTS=ON

# 编译
cmake --build build

# 运行所有测试
cd build && ctest

# 运行特定测试
cd build && ctest -R test_slip

# 详细输出
cd build && ctest -V
```

### 使用 Valgrind 检测内存泄漏
```bash
# 在 Debug 模式下构建
cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
cmake --build build-debug

# 使用 Valgrind 运行测试
cd build-debug
valgrind --leak-check=full --show-leak-kinds=all ./tests/test_slip
```

### 代码覆盖率
```bash
# 使用 gcov/lcov
cmake -B build-cov -G Ninja -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_C_FLAGS="-fprofile-arcs -ftest-coverage"
cmake --build build-cov
cd build-cov && ctest

# 生成覆盖率报告
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## 测试最佳实践

### 1. 测试命名
- 使用描述性名称: `test_function_name_scenario`
- 示例: `test_slip_encode_with_escape_characters`

### 2. 测试结构 (AAA 模式)
```c
void test_example(void) {
    // Arrange - 准备测试数据
    uint8_t input[] = {1, 2, 3};
    uint8_t output[10];
    
    // Act - 执行被测试的函数
    int ret = function_under_test(input, output);
    
    // Assert - 验证结果
    TEST_ASSERT_EQUAL(0, ret);
    TEST_ASSERT_EQUAL(expected_value, output[0]);
}
```

### 3. 测试覆盖原则
- 正常路径 (happy path)
- 边界条件
- 错误处理
- 异常输入

### 4. Mock 使用
对于依赖外部资源的函数，使用 mock 进行隔离测试：
```c
// Mock 串口读取
ssize_t __wrap_serial_read(serial_dev_t *dev, void *buf, size_t len) {
    check_expected(dev);
    check_expected(len);
    return (ssize_t)mock();
}
```

## 持续集成

在 `.github/workflows/build.yml` 中添加测试步骤：
```yaml
- name: Run tests
  run: |
    cmake -B build -G Ninja -DENABLE_TESTS=ON
    cmake --build build
    cd build && ctest --output-on-failure
```

## 贡献测试

欢迎贡献测试用例！请确保：
- [ ] 测试通过
- [ ] 代码覆盖新功能
- [ ] 包含边界条件测试
- [ ] 文档更新 (如果需要)
