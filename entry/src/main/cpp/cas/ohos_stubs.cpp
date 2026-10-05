/* OHOS 平台补丁：补齐 giac/libtommath 在 musl(OHOS) 下缺失的符号
 *
 * 1. arc_en_ciel: plot.cc/mathml.cc 中该定义被 KHICAS/GIAC_HAS_STO_38 条件排除，
 *    但 misc.cc 的 index2rgb 与 plot.cc 的 colormap_color_rgb 仍引用它，这里按
 *    plot.cc 原实现补一份（k 按 126 取模的彩虹色映射）。
 * 2. tommath s_read_*: bn_s_mp_rand_platform.c 在 OHOS(musl, 非 glibc) 下
 *    arc4random/wincsp/getrandom/ltm_rng 分支都不成立，但声明处仍产生引用。
 *    OHOS 提供 /dev/urandom，四个符号统一回落到 urandom 读取实现。
 */
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>

extern "C" {

typedef int mp_err_ohos; /* 与 tommath 的 MP_OKAY=0/MP_ERR=-1 一致 */

/* 与 bn_s_mp_rand_platform.c 的 s_read_urandom 相同实现 */
static mp_err_ohos ohos_read_urandom(void *p, size_t n)
{
   int fd;
   char *q = (char *)p;

   do {
      fd = open("/dev/urandom", O_RDONLY);
   } while ((fd == -1) && (errno == EINTR));
   if (fd == -1) return -1; /* MP_ERR */

   while (n > 0u) {
      ssize_t ret = read(fd, q, n);
      if (ret < 0) {
         if (errno == EINTR) {
            continue;
         }
         close(fd);
         return -1; /* MP_ERR */
      }
      q += ret;
      n -= (size_t)ret;
   }

   close(fd);
   return 0; /* MP_OKAY */
}

mp_err_ohos s_read_arc4random(void *p, size_t n) { return ohos_read_urandom(p, n); }
mp_err_ohos s_read_wincsp(void *p, size_t n)     { return ohos_read_urandom(p, n); }
mp_err_ohos s_read_getrandom(void *p, size_t n)  { return ohos_read_urandom(p, n); }
mp_err_ohos s_read_ltm_rng(void *p, size_t n)    { return ohos_read_urandom(p, n); }

} /* extern "C" */

namespace giac {
void arc_en_ciel(int k, int &r, int &g, int &b) {
    k += 21;
    k %= 126;
    if (k < 0)
        k += 126;
    if (k < 63) {
        if (k < 21) {
            r = 251; g = 0; b = 12 * k; return;
        }
        if (k >= 21 && k < 42) {
            r = 251 - (12 * (k - 21)); g = 0; b = 251; return;
        }
        if (k >= 42 && k < 63) {
            r = 0; g = (k - 42) * 12; b = 251; return;
        }
    } else {
        if (k >= 63 && k < 84) {
            r = 0; g = 251; b = 251 - (k - 63) * 12; return;
        }
        if (k >= 84 && k < 105) {
            r = (k - 84) * 12; g = 251; b = 0; return;
        }
        if (k >= 105 && k < 126) {
            r = 251; g = 251 - (k - 105) * 12; b = 0; return;
        }
    }
}
} // namespace giac
