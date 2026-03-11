//
// Created by xyrls on 2026/3/11.
//

#ifndef ANDROID_OPENCC_LRUCACHE_H
#define ANDROID_OPENCC_LRUCACHE_H

#include <jni.h>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <list>
#include "xxhash.h"

template<typename K, typename V>
class LRUCache {
public:
    [[maybe_unused]] explicit LRUCache(size_t capacity) : cap_(capacity) {}

    V get(const K &key) {
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = map_.find(key);
        if (it == map_.end())
            return V();

        list_.splice(list_.begin(), list_, it->second);

        return it->second->second;
    }

    bool put(const K &key, const V &value) {
        std::unique_lock<std::mutex> lock(mtx_);
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


private:
    size_t cap_;
    std::mutex mtx_;
    std::list<std::pair<K, V>> list_;
    std::unordered_map<K, typename std::list<std::pair<K, V>>::iterator> map_;
};

template
class LRUCache<uint64_t, std::string>;

#endif //ANDROID_OPENCC_LRUCACHE_H
