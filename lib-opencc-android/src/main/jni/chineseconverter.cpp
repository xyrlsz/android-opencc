#include <jni.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
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

static std::mutex &GetConversionMutex() {
    static std::mutex mutex;
    return mutex;
}

extern "C"
jstring
Java_com_xyrlsz_opencc_android_lib_ChineseConverter_nativeConvert(
        JNIEnv *env, jclass type, jstring text_, jstring configFile_,
        jstring absoluteDataFolderPath_) {

    if (env->ExceptionCheck()) {
        return nullptr;
    }

    if (text_ == nullptr || configFile_ == nullptr || absoluteDataFolderPath_ == nullptr) {
        return nullptr;
    }

    jsize textUtfLen = env->GetStringUTFLength(text_);
    jsize configUtfLen = env->GetStringUTFLength(configFile_);
    jsize pathUtfLen = env->GetStringUTFLength(absoluteDataFolderPath_);

    if (textUtfLen <= 0) {
        return text_;  // 空文本直接返回
    }
    if (configUtfLen <= 0 || pathUtfLen <= 0) {
        return text_;
    }

    std::string text, configFile, absoluteDataFolderPath;

    auto copyJniString = [env](jstring value, jsize length, std::string &target) {
        const char *raw = env->GetStringUTFChars(value, nullptr);
        if (raw == nullptr) {
            return false;
        }
        target.assign(raw, static_cast<size_t>(length));
        env->ReleaseStringUTFChars(value, raw);
        return true;
    };

    if (!copyJniString(text_, textUtfLen, text)
            || !copyJniString(configFile_, configUtfLen, configFile)
            || !copyJniString(absoluteDataFolderPath_, pathUtfLen, absoluteDataFolderPath)) {
        return text_;
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
        std::lock_guard<std::mutex> lock(GetConversionMutex());
        ConverterEntry entry = GetGlobalCache().getOrCreate(fullPath);
        std::string result = GetGlobalCache().convertWithCache(entry, text);
        return env->NewStringUTF(result.c_str());
    } catch (const std::exception &e) {
        return env->NewStringUTF(text.c_str());
    }
}

extern "C"
void
Java_com_xyrlsz_opencc_android_lib_ChineseConverter_nativeClearCache(
        JNIEnv *env, jclass type) {
    std::lock_guard<std::mutex> lock(GetConversionMutex());
    ConverterCache &cache = GetGlobalCache();
    std::unique_lock<std::shared_mutex> cacheLock(cache.converterMtx);
    cache.converterMap.clear();
    cache.resultCache.clear();
}
