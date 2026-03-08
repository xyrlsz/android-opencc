#include <jni.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include "Converter.hpp"
#include "Config.hpp"


struct ConverterCache {
    std::unordered_map<std::string, opencc::ConverterPtr> cache;
    mutable std::shared_mutex mtx; // 读写锁

    opencc::ConverterPtr getOrCreate(const std::string &fullPath) {
        // 1. 先尝试共享锁读取 (无阻塞，高并发友好)
        {
            std::shared_lock<std::shared_mutex> lock(mtx);
            auto it = cache.find(fullPath);
            if (it != cache.end()) {
                return it->second;
            }
        }

        // 2. 如果没找到，升级为独占锁进行写入
        std::unique_lock<std::shared_mutex> lock(mtx);
        // 双重检查 (Double Check)，防止其他线程在你等待锁的时候已经创建了
        auto it = cache.find(fullPath);
        if (it != cache.end()) {
            return it->second;
        }

        opencc::Config config;
        opencc::ConverterPtr converter = config.NewFromFile(fullPath);
        cache[fullPath] = converter;
        return converter;
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
    const char *text = env->GetStringUTFChars(text_, nullptr);
    const char *configFile = env->GetStringUTFChars(configFile_, nullptr);
    const char *absoluteDataFolderPath = env->GetStringUTFChars(absoluteDataFolderPath_, nullptr);

    if (!text || !configFile || !absoluteDataFolderPath) {
        if (text) env->ReleaseStringUTFChars(text_, text);
        if (configFile) env->ReleaseStringUTFChars(configFile_, configFile);
        if (absoluteDataFolderPath)
            env->ReleaseStringUTFChars(absoluteDataFolderPath_, absoluteDataFolderPath);
        return text_;
    }

    std::string fullPath;
    fullPath.reserve(strlen(absoluteDataFolderPath) + strlen(configFile) + 1);
    fullPath.append(absoluteDataFolderPath);
    fullPath.push_back('/');
    fullPath.append(configFile);
    try {
        opencc::ConverterPtr converter = GetGlobalCache().getOrCreate(fullPath);

        env->ReleaseStringUTFChars(text_, text);
        env->ReleaseStringUTFChars(configFile_, configFile);
        env->ReleaseStringUTFChars(absoluteDataFolderPath_, absoluteDataFolderPath);

        std::string result = converter->Convert(text);

        return env->NewStringUTF(result.c_str());

    } catch (const std::exception &e) {
        env->ReleaseStringUTFChars(text_, text);
        env->ReleaseStringUTFChars(configFile_, configFile);
        env->ReleaseStringUTFChars(absoluteDataFolderPath_, absoluteDataFolderPath);
        return text_;
    }
}
