#include <jni.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include "xxhash.h"
#include "Converter.hpp"
#include "Config.hpp"

#include "LRUCache.h"

// 预计算 fullPath 的 XXH3 哈希，避免每次转换时重复计算
struct ConverterEntry {
    opencc::ConverterPtr converter;
    uint64_t pathHash;          // fullPath 的 XXH3_64bits 哈希
};

struct ConverterCache {
    std::unordered_map<std::string, ConverterEntry> converterMap;
    mutable std::shared_mutex converterMtx;

    // 扩大缓存容量：2048 -> 8192，最大缓存结果 4096 -> 65536
    LRUCache<uint64_t, CacheEntry> resultCache{8192};

    ConverterEntry getOrCreate(const std::string &fullPath) {
        {
            std::shared_lock<std::shared_mutex> lock(converterMtx);
            auto it = converterMap.find(fullPath);
            if (it != converterMap.end()) {
                return it->second;
            }
        }

        std::unique_lock<std::shared_mutex> lock(converterMtx);
        auto it = converterMap.find(fullPath);
        if (it != converterMap.end()) {
            return it->second;
        }

        opencc::Config config;
        opencc::ConverterPtr converter = config.NewFromFile(fullPath);

        uint64_t pathHash = XXH3_64bits(fullPath.data(), fullPath.size());
        ConverterEntry entry = {converter, pathHash};
        converterMap[fullPath] = entry;
        return entry;
    }

    // 利用 XOR 组合两个独立哈希，零分配、无字符串拼接
    std::string convertWithCache(const ConverterEntry &entry, const std::string &text) {
        // pathHash 已预计算，只需哈希 text，XOR 组合避免堆分配
        uint64_t key = entry.pathHash ^ XXH3_64bits(text.data(), text.size());

        // 快速缓存查找（shared_lock 并发读，无锁竞争）
        auto cached = resultCache.get(key);
        if (!cached.value.empty() && cached.keyStr == text) {
            return cached.value;
        }

        std::string result = entry.converter->Convert(text);

        // 缓存结果（上限 64KB，覆盖绝大多数使用场景）
        if (!result.empty() && result.length() < 65536) {
            resultCache.put(key, {text, result});
        }
        return result;
    }
};

static ConverterCache &GetGlobalCache() {
    static ConverterCache instance;
    return instance;
}

// 栈缓冲区阈值：UTF-8 ≤1024 字节用 GetStringUTFRegion 避免 JNI pinning
// 中文 ~3 字节/字，1024 字节 ≈ 340 字，覆盖大多数短消息
static constexpr jsize kStackBufSize = 1024;

extern "C"
jstring
Java_com_xyrlsz_opencc_android_lib_ChineseConverter_nativeConvert(
        JNIEnv *env, jclass type, jstring text_, jstring configFile_,
        jstring absoluteDataFolderPath_) {

    if (env->ExceptionCheck()) {
        return nullptr;
    }

    // --- 获取字符串长度 ---
    // GetStringUTFLength → 修改版 UTF-8 编码的字节数
    // GetStringLength    → Java char (UTF-16) 的字符数
    // GetStringUTFRegion 的 len 参数是字符数，不是字节数！
    jsize textUtfLen = env->GetStringUTFLength(text_);
    jsize configUtfLen = env->GetStringUTFLength(configFile_);
    jsize pathUtfLen = env->GetStringUTFLength(absoluteDataFolderPath_);

    if (textUtfLen <= 0) {
        return text_;  // 空文本直接返回
    }
    if (configUtfLen <= 0 || pathUtfLen <= 0) {
        return text_;
    }

    // --- 使用栈缓冲区处理小字符串，避免 JNI pinning ---
    // UTF-8 字节数 ≤ kStackBufSize(1024) 用 GetStringUTFRegion 拷贝到栈上
    // 注意：GetStringUTFRegion(env, str, start, charCount, buf) 的 len 参数
    // 是 Java 字符数（GetStringLength），而非 UTF-8 字节数（GetStringUTFLength）
    // 对于大字符串，回退到 GetStringUTFChars
    std::string text, configFile, absoluteDataFolderPath;

    if (textUtfLen <= kStackBufSize) {
        jsize textCharLen = env->GetStringLength(text_);
        char buf[kStackBufSize];
        env->GetStringUTFRegion(text_, 0, textCharLen, buf);
        if (env->ExceptionCheck()) return text_;
        text.assign(buf, static_cast<size_t>(textUtfLen));
    } else {
        const char *raw = env->GetStringUTFChars(text_, nullptr);
        if (!raw) return text_;
        text.assign(raw, static_cast<size_t>(textUtfLen));
        env->ReleaseStringUTFChars(text_, raw);
    }

    if (configUtfLen <= kStackBufSize) {
        jsize configCharLen = env->GetStringLength(configFile_);
        char buf[kStackBufSize];
        env->GetStringUTFRegion(configFile_, 0, configCharLen, buf);
        if (env->ExceptionCheck()) return text_;
        configFile.assign(buf, static_cast<size_t>(configUtfLen));
    } else {
        const char *raw = env->GetStringUTFChars(configFile_, nullptr);
        if (!raw) return text_;
        configFile.assign(raw, static_cast<size_t>(configUtfLen));
        env->ReleaseStringUTFChars(configFile_, raw);
    }

    if (pathUtfLen <= kStackBufSize) {
        jsize pathCharLen = env->GetStringLength(absoluteDataFolderPath_);
        char buf[kStackBufSize];
        env->GetStringUTFRegion(absoluteDataFolderPath_, 0, pathCharLen, buf);
        if (env->ExceptionCheck()) return text_;
        absoluteDataFolderPath.assign(buf, static_cast<size_t>(pathUtfLen));
    } else {
        const char *raw = env->GetStringUTFChars(absoluteDataFolderPath_, nullptr);
        if (!raw) return text_;
        absoluteDataFolderPath.assign(raw, static_cast<size_t>(pathUtfLen));
        env->ReleaseStringUTFChars(absoluteDataFolderPath_, raw);
    }

    // --- 构造 fullPath 并获取/创建 Converter ---
    // 规范化路径：避免 absoluteDataFolderPath 末尾自带 '/' 导致 "//"
    std::string fullPath;
    fullPath.reserve(absoluteDataFolderPath.size() + 1 + configFile.size());
    fullPath = absoluteDataFolderPath;
    if (!fullPath.empty() && fullPath.back() != '/') {
        fullPath += '/';
    }
    fullPath += configFile;

    try {
        ConverterEntry entry = GetGlobalCache().getOrCreate(fullPath);
        std::string result = GetGlobalCache().convertWithCache(entry, text);
        return env->NewStringUTF(result.c_str());
    } catch (const std::exception &e) {
        return env->NewStringUTF(text.c_str());
    }
}
