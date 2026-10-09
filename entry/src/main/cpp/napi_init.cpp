// napi_init.cpp — 梦烬几何 NAPI 模块入口
// 模块名：dreamember
// 导出：
//   casCaseval(expr: string): string  — Giac CAS 求值
//   nativeVersion(): string           — 原生层版本标识（诊断用）
#include "napi/native_api.h"
#include <string>
#include "cas/cas_wrapper.h"

static napi_value CasCaseval(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc < 1 || args[0] == nullptr) {
        napi_throw_error(env, nullptr, "casCaseval requires a string argument");
        return nullptr;
    }
    napi_valuetype vt = napi_undefined;
    napi_typeof(env, args[0], &vt);
    if (vt != napi_string) {
        napi_throw_error(env, nullptr, "casCaseval expects a string");
        return nullptr;
    }
    size_t len = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &len);
    std::string expr(len + 1, '\0');
    napi_get_value_string_utf8(env, args[0], &expr[0], len + 1, &len);
    expr.resize(len);

    std::string result = dreamember::CasCaseval(expr);

    napi_value out;
    napi_create_string_utf8(env, result.c_str(), result.size(), &out);
    return out;
}

static napi_value NativeVersion(napi_env env, napi_callback_info info)
{
    napi_value out;
    napi_create_string_utf8(env, "dreamember-native 0.1.0", NAPI_AUTO_LENGTH, &out);
    return out;
}

// kernel_napi.cpp 提供的注册入口（kernelInput/kernelRender/手势等）
void DreamemberRegisterKernelApis(napi_env env, napi_value exports);

// ratex_napi.cpp 提供的注册入口（RaTeX LaTeX 排版，五十二包）
void DreamemberRegisterRatexApis(napi_env env, napi_value exports);

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        { "casCaseval", nullptr, CasCaseval, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "nativeVersion", nullptr, NativeVersion, nullptr, nullptr, nullptr, napi_default, nullptr },
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    DreamemberRegisterKernelApis(env, exports);
    DreamemberRegisterRatexApis(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module dreamemberModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "dreamember",
    .nm_priv = ((void *)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterDreamemberModule(void)
{
    napi_module_register(&dreamemberModule);
}
