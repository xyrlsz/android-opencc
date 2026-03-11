#include <jni.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include "xxhash.h"
#include "Converter.hpp"
#include "Config.hpp"

#include "LRUCache.h"

struct ConverterCache {
    std::unordered_map<std::string, opencc::ConverterPtr> converterMap;
    mutable std::shared_mutex converterMtx;

    LRUCache<uint64_t, std::string> resultCache{2048};

    opencc::ConverterPtr getOrCreate(const std::string &fullPath) {
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
        converterMap[fullPath] = converter;
        return converter;
    }

    std::string convertWithCache(const std::string &fullPath, const std::string &text) {

        uint64_t key = XXH3_64bits(text.data(), text.size());
        key ^= XXH3_64bits(fullPath.data(), fullPath.size());

        std::string cachedResult = resultCache.get(key);
        if (!cachedResult.empty()) {
            return cachedResult;
        }

        opencc::ConverterPtr converter = getOrCreate(fullPath);
        std::string result = converter->Convert(text);

        if (!result.empty() && result.length() < 4096) {
            resultCache.put(key, result);
        }
        return result;
    }
};

static ConverterCache &GetGlobalCache() {
    static ConverterCache instance;
    return instance;
}

extern "C"
jstring
Java_com_xyrlsz_opencc_android_lib_ChineseConverter_convert(
        JNIEnv *env, jclass type, jstring text_, jstring configFile_,
        jstring absoluteDataFolderPath_) {

    const char *textRaw = env->GetStringUTFChars(text_, nullptr);
    const char *configFileRaw = env->GetStringUTFChars(configFile_, nullptr);
    const char *absoluteDataFolderPathRaw = env->GetStringUTFChars(absoluteDataFolderPath_,
                                                                   nullptr);

    if (!textRaw || !configFileRaw || !absoluteDataFolderPathRaw) {
        if (textRaw) env->ReleaseStringUTFChars(text_, textRaw);
        if (configFileRaw) env->ReleaseStringUTFChars(configFile_, configFileRaw);
        if (absoluteDataFolderPathRaw)
            env->ReleaseStringUTFChars(absoluteDataFolderPath_, absoluteDataFolderPathRaw);
        return text_;
    }

    std::string text = textRaw;
    std::string configFile = configFileRaw;
    std::string absoluteDataFolderPath = absoluteDataFolderPathRaw;

    env->ReleaseStringUTFChars(text_, textRaw);
    env->ReleaseStringUTFChars(configFile_, configFileRaw);
    env->ReleaseStringUTFChars(absoluteDataFolderPath_, absoluteDataFolderPathRaw);

    std::string fullPath = absoluteDataFolderPath + "/" + configFile;

    try {
        std::string result = GetGlobalCache().convertWithCache(fullPath, text);
        return env->NewStringUTF(result.c_str());
    } catch (const std::exception &e) {
        return env->NewStringUTF(text.c_str());
    }
}
