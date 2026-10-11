// 宿主测试桩：最小 NAPI 类型与空实现，仅为编译 ratex_napi.cpp 的
// 纯字符串逻辑（LinearToLatex/GreekAt 等），不执行任何 NAPI 回调。
#ifndef DREAMEMBER_HOST_STUB_NAPI_H
#define DREAMEMBER_HOST_STUB_NAPI_H

#include <cstddef>
#include <cstdint>

typedef void *napi_env;
typedef void *napi_value;
typedef void *napi_callback_info;
typedef void *napi_ref;

enum napi_valuetype {
    napi_undefined = 0, napi_null, napi_boolean, napi_number,
    napi_string, napi_symbol, napi_object, napi_function, napi_external,
    napi_bigint
};

constexpr int napi_default = 0;

typedef struct {
    const char *utf8name;
    void *name;
    void *method;
    void *getter;
    void *setter;
    void *value;
    int attributes;
    void *data;
} napi_property_descriptor;

inline int napi_get_cb_info(napi_env, napi_callback_info, size_t *, napi_value *,
    void **, void **) { return 0; }
inline int napi_get_value_string_utf8(napi_env, napi_value, char *, size_t, size_t *len)
    { if (len) *len = 0; return 0; }
inline int napi_get_value_int32(napi_env, napi_value, int32_t *out) { *out = 0; return 0; }
inline int napi_typeof(napi_env, napi_value, napi_valuetype *t) { *t = napi_undefined; return 0; }
inline int napi_throw_error(napi_env, const char *, const char *) { return 0; }
inline int napi_create_string_utf8(napi_env, const char *, size_t, napi_value *out)
    { *out = nullptr; return 0; }
inline int napi_define_properties(napi_env, void *, size_t, napi_property_descriptor *)
    { return 0; }

#endif
