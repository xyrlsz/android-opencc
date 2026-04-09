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
	implementation 'com.github.xyrlsz:android-opencc:1.3.8'
}
```

# 使用方法
使用中文转换器很容易，只需调用 `ChineseConverter.convert();`

建议使用前调用`ChineseConverter.init(context)`进行初始化，调用 `ChineseConverter.convert()`就可以不传入`context`参数了。

## 支持的转换类型
- HK2S: 繁体中文（香港标准）到简体中文
- HK2T: 繁体中文（香港变体）到繁体中文
- JP2T: 新日文汉字（新字体）到繁体中文字符（旧字体）
- S2HK: 简体中文到繁体中文（香港标准）
- S2T: 简体中文到繁体中文
- S2TW: 简体中文到繁体中文（台湾标准）
- S2TWP: 简体中文到繁体中文（台湾标准）并转换为台湾常用词汇
- T2HK: 繁体中文到繁体中文（香港标准）
- T2S: 繁体中文到简体中文
- T2TW: 繁体中文到繁体中文（台湾标准）
- TW2S: 繁体中文（台湾标准）到简体中文
- T2JP: 繁体中文字符（旧字体）到新日文汉字（新字体）
- TW2T: 繁体中文（台湾标准）到繁体中文
- TW2SP: 繁体中文（台湾标准）到简体中文并转换为中国大陆常用词汇

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