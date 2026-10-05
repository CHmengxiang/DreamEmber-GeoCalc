// ggbfile.cpp — .ggb 容器（zip）读写实现
// 写入：单条目 geogebra.xml，store 方式 + UTF-8 名称标志。
// 读取：从 EOCD 找中央目录，定位条目（store 直接拷贝，deflate 用 zlib 原始解压），
// 兼容真实 GeoGebra .ggb（其条目为 deflate 且带数据描述符——尺寸以中央目录为准）。
#include "kernel/ggbfile.h"
#include <algorithm>
#include <cstring>
#include <zlib.h>

namespace dreamember {

namespace {

const char *kEntryName = "geogebra.xml";

void PutU16(std::string &s, unsigned v)
{
    s.push_back((char)(v & 0xFF));
    s.push_back((char)((v >> 8) & 0xFF));
}

void PutU32(std::string &s, unsigned long v)
{
    s.push_back((char)(v & 0xFF));
    s.push_back((char)((v >> 8) & 0xFF));
    s.push_back((char)((v >> 16) & 0xFF));
    s.push_back((char)((v >> 24) & 0xFF));
}

unsigned GetU16(const std::string &s, size_t off)
{
    if (off + 2 > s.size()) return 0;
    return (unsigned)(unsigned char)s[off] | ((unsigned)(unsigned char)s[off + 1] << 8);
}

unsigned long GetU32(const std::string &s, size_t off)
{
    if (off + 4 > s.size()) return 0;
    return (unsigned long)(unsigned char)s[off]
         | ((unsigned long)(unsigned char)s[off + 1] << 8)
         | ((unsigned long)(unsigned char)s[off + 2] << 16)
         | ((unsigned long)(unsigned char)s[off + 3] << 24);
}

unsigned long Crc32(const std::string &data)
{
    uLong crc = crc32(0L, Z_NULL, 0);
    if (!data.empty()) {
        crc = crc32(crc, (const Bytef *)data.data(), (uInt)data.size());
    }
    return (unsigned long)crc;
}

} // namespace

bool MakeGgb(const std::string &xml, std::string &outZip)
{
    outZip.clear();
    const unsigned long crc = Crc32(xml);
    const unsigned long size = xml.size();
    const unsigned nameLen = (unsigned)strlen(kEntryName);

    // local file header + 数据（store）
    PutU32(outZip, 0x04034b50);
    PutU16(outZip, 20);        // version needed
    PutU16(outZip, 0x0800);    // flags: UTF-8 文件名
    PutU16(outZip, 0);         // method: store
    PutU16(outZip, 0);         // mod time
    PutU16(outZip, 0x21);      // mod date: 1980-01-01
    PutU32(outZip, crc);
    PutU32(outZip, size);
    PutU32(outZip, size);
    PutU16(outZip, nameLen);
    PutU16(outZip, 0);         // extra len
    outZip += kEntryName;
    outZip += xml;

    // central directory
    std::string cd;
    PutU32(cd, 0x02014b50);
    PutU16(cd, 20);            // version made by
    PutU16(cd, 20);            // version needed
    PutU16(cd, 0x0800);
    PutU16(cd, 0);             // method: store
    PutU16(cd, 0);             // time
    PutU16(cd, 0x21);
    PutU32(cd, crc);
    PutU32(cd, size);
    PutU32(cd, size);
    PutU16(cd, nameLen);
    PutU16(cd, 0);             // extra
    PutU16(cd, 0);             // comment
    PutU16(cd, 0);             // disk start
    PutU16(cd, 0);             // internal attrs
    PutU32(cd, 0);             // external attrs
    PutU32(cd, 0);             // local header offset
    cd += kEntryName;

    outZip += cd;
    const size_t cdOffset = outZip.size() - cd.size();
    PutU32(outZip, 0x06054b50);   // EOCD
    PutU16(outZip, 0);            // disk
    PutU16(outZip, 0);            // cd disk
    PutU16(outZip, 1);            // entries this disk
    PutU16(outZip, 1);            // entries total
    PutU32(outZip, cd.size());
    PutU32(outZip, cdOffset);
    PutU16(outZip, 0);            // comment len
    return true;
}

bool ParseGgb(const std::string &zip, std::string &outXml)
{
    outXml.clear();
    if (zip.size() < 22) return false;

    // 1) 从尾部向前找 EOCD（容忍注释）
    size_t eocd = std::string::npos;
    const size_t lo = zip.size() > 22 + 65535 ? zip.size() - 22 - 65535 : 0;
    for (size_t i = zip.size() - 22 + 1; i-- > lo;) {
        if (GetU32(zip, i) == 0x06054b50) {
            eocd = i;
            break;
        }
    }
    if (eocd == std::string::npos) return false;

    const unsigned entries = GetU16(zip, eocd + 10);
    const unsigned long cdSize = GetU32(zip, eocd + 12);
    const unsigned long cdOff = GetU32(zip, eocd + 16);
    if (cdOff + cdSize > zip.size() || entries == 0) return false;

    // 2) 扫中央目录找 geogebra.xml；找不到则退而接受首个 .xml 条目
    unsigned method = 0;
    unsigned long compSize = 0, uncompSize = 0, localOff = 0;
    bool found = false, hasXml = false;
    for (unsigned i = 0, p = (unsigned)cdOff;
         i < entries && p + 46 <= cdOff + cdSize; ++i) {
        if (GetU32(zip, p) != 0x02014b50) return false;
        const unsigned m = GetU16(zip, p + 10);
        const unsigned long cs = GetU32(zip, p + 20);
        const unsigned long us = GetU32(zip, p + 24);
        const unsigned nameLen = GetU16(zip, p + 28);
        const unsigned extraLen = GetU16(zip, p + 30);
        const unsigned cmtLen = GetU16(zip, p + 32);
        const unsigned long loff = GetU32(zip, p + 42);
        if (p + 46 + nameLen > zip.size()) return false;
        std::string name = zip.substr(p + 46, nameLen);
        if (name == kEntryName) {
            method = m; compSize = cs; uncompSize = us; localOff = loff;
            found = true;
            break;
        }
        if (!hasXml && name.size() > 4 &&
            name.compare(name.size() - 4, 4, ".xml") == 0) {
            method = m; compSize = cs; uncompSize = us; localOff = loff;
            hasXml = true;
        }
        p += 46 + nameLen + extraLen + cmtLen;
    }
    if (!found && !hasXml) return false;

    // 3) 定位数据区：本地头里的名称/extra 长度可能与中央目录不同，按本地头算
    if (localOff + 30 > zip.size() || GetU32(zip, localOff) != 0x04034b50) return false;
    const unsigned lNameLen = GetU16(zip, localOff + 26);
    const unsigned lExtraLen = GetU16(zip, localOff + 28);
    const size_t dataOff = localOff + 30 + lNameLen + lExtraLen;
    if (dataOff + compSize > zip.size()) return false;

    if (method == 0) {
        outXml = zip.substr(dataOff, compSize);
        return true;
    }
    if (method != 8) return false;   // 仅支持 store / deflate

    // deflate（zip 用原始 deflate 流，windowBits=-15）
    if (uncompSize == 0 || uncompSize > 64u * 1024 * 1024) return false;
    outXml.assign((size_t)uncompSize, '\0');
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, -15) != Z_OK) return false;
    zs.next_in = (Bytef *)(const_cast<char *>(zip.data()) + dataOff);
    zs.avail_in = (uInt)compSize;
    zs.next_out = (Bytef *)&outXml[0];
    zs.avail_out = (uInt)uncompSize;
    const int rc = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    if (rc != Z_STREAM_END) {
        outXml.clear();
        return false;
    }
    outXml.resize(zs.total_out);
    return true;
}

} // namespace dreamember
