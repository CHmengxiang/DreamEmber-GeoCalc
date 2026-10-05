// cas_wrapper.cpp — Giac CAS 的最小 NAPI 包裹
// 暴露 caseval(expr: string, context?: object): string
// 语义对齐 上游官方 giac.cwrap('caseval') 的用法：输入表达式串，返回求值结果串。
#include "cas_wrapper.h"

#include <cmath>
#include <cstdlib>
#include "kernel/expr.h"   // PreprocessInput / ReplaceAns / SetAnsValue（第七包）

#ifdef WITH_GIAC
#include <pthread.h>
#include <signal.h>
#include <setjmp.h>
#include <string.h>
#include <giac/config.h>
#include <giac/giac.h>
// hilog 必须在 giac 头之后引入：其 LOG_TAG/LOG_DOMAIN 宏会撞坏 ti89.h 的枚举；
// 本文件只用 OH_LOG_Print 显式传参，不受 undef 影响
#include <hilog/log.h>
#undef LOG_TAG
#undef LOG_DOMAIN

using namespace giac;
static context *g_context = nullptr;

static context *get_context() {
    if (g_context == nullptr) {
        g_context = new context;
    }
    return g_context;
}

// 阶段标记：Factor 真机闪退静态排查三轮无果（do_factor/sqff/linearfind 全为
// 纯整数路径），一旦再有故障，日志直接给出死在 init/parse/eval/print 哪一步
static volatile sig_atomic_t g_cas_stage = 0;   // 0=空 1=init 2=parse 3=eval 4=print
static const char *CasStageName(int s)
{
    switch (s) {
    case 1: return "init";
    case 2: return "parse";
    case 3: return "eval";
    case 4: return "print";
    default: return "?";
    }
}

static std::string CasEval(const std::string &expr)
{
    try {
        context *ctx = get_context();
        static bool inited = false;
        if (!inited) {
            g_cas_stage = 1;
            // 关闭 giac 内部多线程：真机 sysconf 报告多核后部分算法会开工作
            // 线程（do_thread_fftmult / do_convert_from 等），它们既不在本文件
            // 的信号防护内也没有大栈；上游官方 wasm 构建同样从不启用
            threads_allowed = false;
            threads = 1;
            // C 级 caseval 走全局 context（shell 开关只对它有意义）
            caseval("shell off");
            caseval("init geogebra");
            // geogebra 模式必须同时落到实际求值的 context：giac 的 withsqrt
            // 默认为 true（factor 会进根式扩域分解的重路径），
            // init_geogebra 才把它关成 上游 语义（有理数域分解）
            init_geogebra(1, ctx);
            inited = true;
        }
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xE520, "CasEval", "cas in: %{public}s", expr.c_str());
        g_cas_stage = 2;
        gen e(expr, ctx);
        g_cas_stage = 3;
        gen r = eval(e, 1, ctx);
        g_cas_stage = 4;
        std::string out = r.print(ctx);
        OH_LOG_Print(LOG_APP, LOG_INFO, 0xE520, "CasEval", "cas out: %{public}s", out.c_str());
        g_cas_stage = 0;
        return out;
    } catch (const std::exception &ex) {
        g_cas_stage = 0;
        return std::string("error: ") + ex.what();
    } catch (...) {
        g_cas_stage = 0;
        return "error: unknown CAS failure";
    }
}

// giac 深递归 + 未知崩点（Factor 真机闪退，静态路径核查无果）三保险：
// 1) 求值固定在 32MB 栈的工作线程（防 UI 线程栈溢出）；
// 2) 工作线程内武装 SIGSEGV/SIGBUS/SIGFPE/SIGILL/SIGTRAP/SIGABRT——含 aarch64
//    上 __builtin_trap/udf 的 brk 陷阱与 abort()（前一轮只护了四种信号，仍
//    整进程闪退，这批是漏网嫌疑最大的）。handler 跑在独立 sigaltstack 上，
//    栈溢出型 SIGSEGV 也能救；求值完即恢复旧 handler，不长期占进程信号表；
// 3) 其他线程的信号不在防护内：恢复默认处理后重发，不掩盖真实问题。
struct CasJob {
    const std::string *in;
    std::string *out;
};

static sigjmp_buf g_cas_env;
static volatile sig_atomic_t g_cas_armed = 0;
static pthread_t g_cas_tid;
static char g_cas_altstack[131072];   // musl SIGSTKSZ=8K 太小，独立备用栈给 128K

static void CasSignalHandler(int sig, siginfo_t *, void *)
{
    if (!g_cas_armed || !pthread_equal(pthread_self(), g_cas_tid)) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sigaction(sig, &sa, nullptr);
        raise(sig);
        return;
    }
    g_cas_armed = 0;
    siglongjmp(g_cas_env, sig == 0 ? 99 : sig);
}

static const int kCasSignals[] = { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGTRAP, SIGABRT };
static struct sigaction g_cas_old[sizeof(kCasSignals) / sizeof(kCasSignals[0])];

static void InstallCasGuards()
{
    stack_t st;
    st.ss_sp = g_cas_altstack;
    st.ss_size = sizeof(g_cas_altstack);
    st.ss_flags = 0;
    sigaltstack(&st, nullptr);
    for (size_t i = 0; i < sizeof(kCasSignals) / sizeof(kCasSignals[0]); ++i) {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = CasSignalHandler;
        sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        if (sigaction(kCasSignals[i], nullptr, &g_cas_old[i]) == 0)
            sigaction(kCasSignals[i], &sa, nullptr);
    }
}

static void RestoreCasGuards()
{
    for (size_t i = 0; i < sizeof(kCasSignals) / sizeof(kCasSignals[0]); ++i)
        sigaction(kCasSignals[i], &g_cas_old[i], nullptr);
}

static void *CasThreadEntry(void *p)
{
    CasJob *j = static_cast<CasJob *>(p);
    InstallCasGuards();
    g_cas_tid = pthread_self();
    const int sig = sigsetjmp(g_cas_env, 1);
    if (sig == 0) {
        g_cas_armed = 1;
        *j->out = CasEval(*j->in);
        g_cas_armed = 0;
    } else {
        // 先恢复旧 handler 再收尾：handler 路径越短越好，后续动作都在正常栈上
        RestoreCasGuards();
        OH_LOG_Print(LOG_APP, LOG_ERROR, 0xE520, "CasEval",
                     "cas fault sig=%{public}d stage=%{public}s expr=%{public}s",
                     sig, CasStageName(g_cas_stage), j->in->c_str());
        *j->out = std::string("error: CAS 内部错误(信号 ") + std::to_string(sig) +
                  "，阶段 " + CasStageName(g_cas_stage) + ")";
    }
    RestoreCasGuards();
    return nullptr;
}

static std::string CasEvalSafe(const std::string &expr)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 32u << 20);
    std::string out;
    CasJob job = { &expr, &out };
    pthread_t tid;
    const int rc = pthread_create(&tid, &attr, CasThreadEntry, &job);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        return CasEval(expr);   // 起线程失败：回退当前线程求值
    }
    pthread_join(tid, nullptr);
    return out;
}

// 把形如 Solve(...) 命令位置上的首字母大写词降为小写。
// 简化版 Ggb2giac：只处理串首或 ")" "(" "," 后跟的标识符，
// 不触碰 "..." 字符串内容与变量（变量保持原大小写）。
static std::string ToLowerCommands(const std::string &expr)
{
    std::string out;
    out.reserve(expr.size());
    bool at_cmd_pos = true; // 串首即命令位
    for (size_t i = 0; i < expr.size(); ++i) {
        char c = expr[i];
        if ((at_cmd_pos || c == '(' || c == ',' || c == ' ') && isupper(static_cast<unsigned char>(c))) {
            // 命令位上的大写字母 -> 小写
            if (at_cmd_pos) {
                out.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
                at_cmd_pos = false;
                continue;
            }
        }
        if (c == '(' || c == ',')
            at_cmd_pos = true;
        else if (c == '"' ) {
            // 字符串原样拷贝到闭合引号
            out.push_back(c);
            ++i;
            for (; i < expr.size() && expr[i] != '"'; ++i)
                out.push_back(expr[i]);
            if (i < expr.size())
                out.push_back('"');
            at_cmd_pos = false;
            continue;
        }
        else if (!isspace(static_cast<unsigned char>(c)))
            at_cmd_pos = false;
        out.push_back(c);
    }
    return out;
}
#endif

namespace dreamember {

// CAS 结果若为纯数值（含 "3/2" 分数形式）则回填 ans（上游：CAS 求值更新上一答案）
static void UpdateAnsFromCas(const std::string &result)
{
    if (result.empty()) return;
    char *end = nullptr;
    const double v = strtod(result.c_str(), &end);
    if (end != nullptr && *end == '\0') {
        if (std::isfinite(v)) SetAnsValue(v);
        return;
    }
    const size_t slash = result.find('/');
    if (slash == std::string::npos || slash == 0 || slash + 1 >= result.size()) return;
    char *e1 = nullptr, *e2 = nullptr;
    const std::string ps = result.substr(0, slash);
    const std::string qs = result.substr(slash + 1);
    const double p = strtod(ps.c_str(), &e1);
    const double q = strtod(qs.c_str(), &e2);
    if (e1 != nullptr && *e1 == '\0' && e2 != nullptr && *e2 == '\0' && q != 0) {
        const double v2 = p / q;
        if (std::isfinite(v2)) SetAnsValue(v2);
    }
}

// giac 对命令名大小写敏感（内置表是小写 "solve" 等），而本应用的命令名
// 是首字母大写（Solve/Derivative/...），上游 正式流程里由 Ggb2giac 映射表先把
// 命令转成小写再送 caseval。N0 冒烟面板直接把用户输入送给 giac，因此这里做
// 兜底：整串解析失败时按 Ggb2giac 的思路把每个命令样式的大写词降为小写重试。
std::string CasCaseval(const std::string &expr)
{
    // 与代数输入同规则：$ / 循环小数 / 带分数预解析 + ans 替换（第七包）
    const std::string pre = ReplaceAns(PreprocessInput(expr));
#ifdef WITH_GIAC
    std::string result = CasEvalSafe(pre);
    // 判据：解析失败时 gen(string) 会把输入原样包成字符串返回（print 加引号），
    // 或返回 "Solve" 这类纯命令名。识别"没被求值"的特征并降级重试。
    bool unevaluated = (!result.empty() && result.front() == '"') ||
                       result == pre || result == pre.substr(0, pre.find('('));
    if (unevaluated && !pre.empty() && islower(pre[0]) == 0) {
        std::string lowered = ToLowerCommands(pre);
        if (lowered != pre) {
            std::string retry = CasEvalSafe(lowered);
            if (!retry.empty() && retry.front() != '"') {
                UpdateAnsFromCas(retry);
                return retry;
            }
        }
    }
    UpdateAnsFromCas(result);
    return result;
#else
    // giac 源码未就位时的占位实现（N0 骨架期）
    return "cas-not-ready: " + pre;
#endif
}

} // namespace dreamember
