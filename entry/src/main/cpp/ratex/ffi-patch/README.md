# RaTeX FFI OpenHarmony 补丁

`prebuilt/` 下的 `libratex_ffi.so`（arm64-v8a / x86_64）由 RaTeX 上游源码加本补丁交叉编译而来。

## 上游基线

- 仓库：https://github.com/erweixin/RaTeX
- 基线提交：`776c1d37bafa3bf445a0ab9377c55fe77f7a0133`（2026-09-23，"Merge pull request #157 from erweixin/feat-test"）
- 补丁文件：`ratex-ffi-ohos.patch`（SHA256 `9fe909eb39f4c94e3abd45ef68f6641afdf2a230d4e7a7496f1babeede83b208`）

## 补丁内容

- `Cargo.toml`：crate-type 追加 `cdylib`，加入 OpenHarmony 交叉编译所需配置
- `src/lib.rs`：新增约 180 行导出接口（供 `ratex_napi.cpp` 调用的 C ABI）

## 重新编译步骤

```bash
git clone https://github.com/erweixin/RaTeX.git
cd RaTeX
git checkout 776c1d37bafa3bf445a0ab9377c55fe77f7a0133
git apply /path/to/ratex-ffi-ohos.patch

# OpenHarmony 目标（需 rustup target add aarch64-unknown-linux-ohos）
export CARGO_TARGET_AARCH64_UNKNOWN_LINUX_OHOS_LINKER="<SDK>/llvm/lld"
# 或使用 ohos-rust 工具链包装脚本，按当时 SDK 版本调整
cargo build --release --target aarch64-unknown-linux-ohos -p ratex-ffi
# 产物 target/aarch64-unknown-linux-ohos/release/libratex_ffi.so
# 复制到 prebuilt/arm64-v8a/libratex_ffi.so，x86_64 同理（target x86_64-unknown-linux-ohos）
```

## 验证

替换 so 后全量构建 HAP，跑公式渲染用例（分数/根号/上下标/矩阵/积分限），与 prebuilt 现有 so 行为对比。
