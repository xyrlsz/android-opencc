//
// Created by xyrls on 2026/3/11.
// Optimized: shared_mutex for concurrent reads, hash-based key for cache efficiency.
//

#ifndef ANDROID_OPENCC_LRUCACHE_H
#define ANDROID_OPENCC_LRUCACHE_H

#include <jni.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <list>
#include "xxhash.h"

struct CacheEntry {
    std::string keyStr;   // 原始 key (仅用于哈希碰撞校验)
    std::string value;    // 转换结果
};

template<typename K, typename V>
class LRUCache {
public:
    [[maybe_unused]] explicit LRUCache(size_t capacity) : cap_(capacity) {}

    // 读操作：shared_lock 允许并发读取
    V get(const K &key) {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        auto it = map_.find(key);
        if (it == map_.end())
            return V();
        return it->second->second;
    }

    // 写操作：unique_lock 独占写入
    bool put(const K &key, const V &value) {
        std::unique_lock<std::shared_mutex> lock(mtx_);
        auto it = map_.find(key);

        if (it != map_.end()) {
            it->second->second = value;
            list_.splice(list_.begin(), list_, it->second);
            return true;
        }

        if (list_.size() >= cap_) {
            auto last = list_.back();
            map_.erase(last.first);
            list_.pop_back();
        }

        list_.push_front({key, value});
        map_[key] = list_.begin();
        return false;
    }

    // 返回当前缓存条目数
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return list_.size();
    }

private:
    size_t cap_;
    mutable std::shared_mutex mtx_;
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
};

template
class LRUCache<uint64_t, CacheEntry>;

#endif //ANDROID_OPENCC_LRUCACHE_H
