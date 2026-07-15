# 简介

[OPENCC](https://github.com/BYVoid/OpenCC) 的 Android 移植版本，这是一个用于转换简体中文和繁体中文的库。此外，在中国大陆简体中文、台湾繁体中文和香港繁体中文之间转换时，它还采用了地区词汇和术语进行互换。

## 注意
此项目使用 git 子模块从 OpenCC 下载源代码，请在克隆此项目时使用 --recursive 标志

```
git submodule update --init --recursive  
```

## 示例
```
滑鼠裡面的矽二極體壞了，導致游標解析度降低。
``` 
在台湾繁体中文中
将转换为 
```
鼠标里面的硅二极管坏了，导致光标分辨率降低。
```
在简体中文中并使用中国大陆术语

# 安装

在您的根目录 build.gradle 中添加以下内容：
```
allprojects {
	repositories {
	...
	    maven { url 'https://jitpack.io' }
	}
}
```

```
// 添加依赖
dependencies {
    ...
	implementation 'com.github.xyrlsz:android-opencc:1.4.1'
}
```

# 使用方法
使用中文转换器很容易，只需调用 `ChineseConverter.convert();`

建议使用前调用`ChineseConverter.init(context)`进行初始化，调用 `ChineseConverter.convert()`就可以不传入`context`参数了。

## 支持的转换类型

[详情](https://github.com/BYVoid/OpenCC#configurations-配置文件)

### 生产可用
- `S2T` — **简体中文** → **OpenCC 标准繁体**
- `T2S` — **OpenCC 标准繁体** → **简体中文**
- `S2TW` — **简体中文** → **台湾正体**
- `TW2S` — **台湾正体** → **简体中文**
- `S2HK` — **简体中文** → **香港繁体**
- `HK2S` — **香港繁体** → **简体中文**
- `S2TWP` — **简体中文** → **台湾正体（含台湾常用词汇）**
- `TW2SP` — **台湾正体** → **简体中文（含中国大陆常用词汇）**
- `T2TW` — **OpenCC 标准繁体** → **台湾正体**
- `TW2T` — **台湾正体** → **OpenCC 标准繁体**
- `T2HK` — **OpenCC 标准繁体** → **香港繁体**
- `HK2T` — **香港繁体** → **OpenCC 标准繁体**

### 开发中
- `S2HKP` — **简体中文** → **香港繁体（含香港常用词汇）**
- `HK2SP` — **香港繁体** → **简体中文（含中国大陆常用词汇）**

### 实验性（日文新旧字体）
- `T2JP` — **日文旧字体（Kyūjitai）** → **日文新字体（Shinjitai）**
- `JP2T` — **日文新字体（Shinjitai）** → **日文旧字体（Kyūjitai）**，并将少量日文词汇转换为对应中文

# 说明

android-opencc 利用原始的 OpenCC 项目并通过 JNI 调用原生代码，文本短语字典文件打包在 assets 文件夹中。Android NDK 不提供直接从 assets 文件夹创建和读取文件流的方法，因此字典文件在第一次调用 `ChineseConverter.convert()` 时会被复制到应用程序数据文件夹中。

相对于[原版](https://github.com/qichuan/android-opencc)，进行了一定的性能优化，并支持16KB页面大小。需要API Level 21以上。

> [!TIP]
> 如果您需要更新 assets 文件夹中的字典文件，请记住调用 `ChineseConverter.clearDictDataFolder()` 一次以清除旧的字典文件，以便新的字典文件在下次 `ChineseConverter.convert()` 调用时生效。

# 编译

您需要 Android NDK 进行编译，请下载 [NDK](http://developer.android.com/ndk/downloads/index.html) 并在 `local.properties` 文件中配置 NDK 路径。


# 参考资料
- https://github.com/BYVoid/OpenCC
- https://github.com/gelosie/OpenCC/tree/master/iOS