# 词典编译

脚本会自动检测系统中已安装的 OpenCC CLI 工具（`opencc_dict` 和 `opencc`），无需从源代码编译。

## 前置要求

安装 OpenCC CLI（安装后 `opencc_dict` 和 `opencc` 命令会加入 PATH）：

### 包管理器

| 平台 | 命令 |
|------|------|
| Windows (WinGet) | `winget install opencc` |
| macOS (Homebrew) | `brew install opencc` |
| Debian / Ubuntu | `apt install opencc` |
| Fedora | `dnf install opencc` |
| Arch Linux | `pacman -S opencc` |

### Node.js CLI

```bash
npm install -g opencc          # 仅 CLI
npm install -g opencc opencc-jieba  # CLI + Jieba 分词插件
```

### Python

```bash
pip install opencc
```

### 预编译二进制文件

从 [OpenCC Releases](https://github.com/BYVoid/OpenCC/releases) 下载最新版：

- **Windows (x86_64)**: `OpenCC-1.4.1-windows-x64-portable.zip`（需要安装 [VC++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)）
- **Debian/Ubuntu**: `opencc-1.4.1-1-deb-amd64.zip` 或 `opencc-1.4.1-1-deb-arm64.zip`（内含 `opencc`、`opencc-jieba`、`libopencc*` 的 deb 包）

## 使用方法

```bash
python3 generate_dicts.py
```

可选参数：
- `--force` — 强制重新生成所有字典
- `--opencc PATH` — 指定 opencc CLI 路径（用于生成 STPhrases_GeneratedFromRegionalPhrases）
- `--build-dir DIR` — 中间文件工作目录（默认：build）
- `--no-config-copy` — 跳过复制配置文件