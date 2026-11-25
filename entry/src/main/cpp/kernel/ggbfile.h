// ggbfile.h — .ggb 文件容器（zip）读写（N4）
// .ggb = zip 包内的 geogebra.xml（GGB 标准条目名）。
// 写入用 store 方式（不压缩，最小实现保证正确性）；
// 读取支持 store 与 deflate，可打开 GeoGebra 官方生成的真实 .ggb 文件。
#ifndef DREAMEMBER_KERNEL_GGBFILE_H
#define DREAMEMBER_KERNEL_GGBFILE_H

#include <map>
#include <string>
#include <vector>

namespace dreamember {

// 173包 media 条目名前缀（写侧 <file name> 引用与读侧条目归属共用）
extern const char *const kGgbMediaPrefix;

// 把 geogebra.xml 打包成 .ggb（zip 容器）
bool MakeGgb(const std::string &xml, std::string &outZip);
// 从 zip 提取 geogebra.xml；找不到该条目返回 false
bool ParseGgb(const std::string &zip, std::string &outXml);

// 158包 媒体版：xml 之外追加 media 条目（键=对象标签，条目名
// dreamember_media_<键>，store 方式——图片字节已压缩）。xml 条目仍居首
bool MakeGgbMedia(
    const std::string &xml,
    const std::map<std::string, std::vector<unsigned char>> &media,
    std::string &outZip);
// 媒体版读取：geogebra.xml + 全部 media 条目；xml 缺失返回 false
//（无 media 条目视为空集）
bool ParseGgbMedia(
    const std::string &zip, std::string &outXml,
    std::map<std::string, std::vector<unsigned char>> &media);

} // namespace dreamember
#endif // DREAMEMBER_KERNEL_GGBFILE_H
