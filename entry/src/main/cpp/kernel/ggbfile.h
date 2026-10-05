// ggbfile.h — .ggb 文件容器（zip）读写（N4）
// .ggb = zip 包内的 geogebra.xml（GGB 标准条目名）。
// 写入用 store 方式（不压缩，最小实现保证正确性）；
// 读取支持 store 与 deflate，可打开 GeoGebra 官方生成的真实 .ggb 文件。
#ifndef DREAMEMBER_KERNEL_GGBFILE_H
#define DREAMEMBER_KERNEL_GGBFILE_H

#include <string>

namespace dreamember {

// 把 geogebra.xml 打包成 .ggb（zip 容器）
bool MakeGgb(const std::string &xml, std::string &outZip);
// 从 zip 提取 geogebra.xml；找不到该条目返回 false
bool ParseGgb(const std::string &zip, std::string &outXml);

} // namespace dreamember
#endif // DREAMEMBER_KERNEL_GGBFILE_H
