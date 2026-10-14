// 七十包②回归：命令名插入吸收边界（名左不吸、名尾吸槽、名内吸名左）
#include "kernel/app.h"
#include <cstdio>
#include <string>
static int fails = 0;
static int total = 0;
static void chk(bool ok, const char *msg) {
  total++;
  if (!ok) { fails++; printf("FAIL: %s\n", msg); }
}
using dreamember::Kernel;
int main() {
  Kernel k;
  // 已填 Integral 正常提交（回归）
  chk(k.input("Integral(x,1,6)"), "Integral(x,1,6) ok");
  chk(k.lastError.empty(), "no err");
  // 命令串破坏态报新文案（回归，大小写不敏感）
  chk(!k.input("2Integral(x,1,6)"), "2Integral rejected");
  printf("err=[%s]\n", k.lastError.c_str());
  chk(k.lastError.find("指令") != std::string::npos, "cmd-only message");
  k.lastError.clear();
  chk(!k.input("Integral(x,1,6)x"), "Integral...x rejected");
  chk(k.lastError.find("指令") != std::string::npos, "cmd-only message 2");
  k.lastError.clear();
  // 匿名函数语义（回归）
  k.input("f1(x)=x^2");
  chk(k.input("a1=Integral(x^2,1,6)"), "Integral anon fn");
  printf("testN: %d checks, %d failed\n", total, fails);
  return fails != 0;
}
