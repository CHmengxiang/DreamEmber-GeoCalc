// cas_wrapper.h — Giac CAS 最小包裹接口
#ifndef DREAMEMBER_CAS_WRAPPER_H
#define DREAMEMBER_CAS_WRAPPER_H

#include <string>

namespace dreamember {

// 求值一个 CAS 表达式，返回其字符串表示（对齐 giac caseval 语义）。
// WITH_GIAC 未定义时返回 "cas-not-ready: <expr>" 占位。
std::string CasCaseval(const std::string &expr);

} // namespace dreamember

#endif // DREAMEMBER_CAS_WRAPPER_H
