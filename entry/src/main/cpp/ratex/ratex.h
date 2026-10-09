// ratex.h — RaTeX C ABI 最小声明（与 ratex-ffi v0.1.14 ratex-ffi/src/lib.rs 对齐）
// 实现位于预编译 libratex_ffi.so（Rust，双 ABI 各一份）
#ifndef DREAMEMBER_RATEX_H
#define DREAMEMBER_RATEX_H

#include <cstddef>
#include <cstdint>

extern "C" {

typedef struct {
    float r;
    float g;
    float b;
    float a;
} RatexColor;

typedef struct {
    size_t struct_size;      // 必须先填 sizeof(RatexOptions)
    int display_mode;        // 0 = inline（行内），1 = display（块级）
    const RatexColor *color; // 可为 nullptr（默认黑）
} RatexOptions;

typedef struct {
    char *data;       // 成功时指向 DisplayList JSON（UTF-8），失败为 nullptr
    int error_code;   // 0 = 成功
} RatexResult;

// 解析 LaTeX 并排版，返回 DisplayList JSON（em 坐标，基线 y=height）。
// 返回后必须用 ratex_free_display_list 释放 data。
RatexResult ratex_parse_and_layout(const char *latex, const RatexOptions *opts);

void ratex_free_display_list(char *ptr);

// 线程局部最近错误信息（error_code != 0 时有效）
const char *ratex_get_last_error(void);

} // extern "C"

#endif // DREAMEMBER_RATEX_H
