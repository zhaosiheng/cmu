//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_hash_table.cpp
//
// Identification: src/container/hash/extendible_hash_table.cpp
//
// Copyright (c) 2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cstdlib>
#include <functional>
#include <list>
#include <utility>

#include "container/hash/extendible_hash_table.h"
#include "storage/page/page.h"

namespace bustub {

template <typename K, typename V>
ExtendibleHashTable<K, V>::ExtendibleHashTable(size_t bucket_size)
    : global_depth_(0), bucket_size_(bucket_size), num_buckets_(1) {
    dir_.push_back(std::make_shared<Bucket>(bucket_size));
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::IndexOf(const K& key) -> size_t {
    int mask = (1 << global_depth_) - 1;
    return std::hash<K>()(key) & mask;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetGlobalDepth() const -> int {
    std::scoped_lock<std::mutex> lock(latch_);
    return GetGlobalDepthInternal();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetGlobalDepthInternal() const -> int {
    return global_depth_;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetLocalDepth(int dir_index) const -> int {
    std::scoped_lock<std::mutex> lock(latch_);
    return GetLocalDepthInternal(dir_index);
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetLocalDepthInternal(int dir_index) const -> int {
    return dir_[dir_index]->GetDepth();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetNumBuckets() const -> int {
    std::scoped_lock<std::mutex> lock(latch_);
    return GetNumBucketsInternal();
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::GetNumBucketsInternal() const -> int {
    return num_buckets_;
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Find(const K& key, V& value) -> bool {
    std::scoped_lock<std::mutex> lock(latch_);
    size_t index = IndexOf(key);
    return dir_[index]->Find(key, value);
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Remove(const K& key) -> bool {
    std::scoped_lock<std::mutex> lock(latch_);
    size_t index = IndexOf(key);
    return dir_[index]->Remove(key);
}

template <typename K, typename V>
void ExtendibleHashTable<K, V>::Insert(const K& key, const V& value) {
    std::scoped_lock<std::mutex> lock(latch_);
    size_t index = IndexOf(key);
    auto ptr = dir_[index];
    bool rs = ptr->Insert(key, value);
    if (rs) {//success
      //global_depth_ = max(dir_[index]->GetDepth , global_depth_);
        return;
    }
    else {//fail
        if (ptr->GetDepth() == global_depth_) {//L==G
            global_depth_++;
            ptr->IncrementDepth();
            //split
            size_t cap = dir_.size();
            dir_.resize(cap * 2);
            for (size_t i = 0; i < cap; i++) {
                dir_[i + cap] = dir_[i];
            }
            //redistribute
            std::shared_ptr<Bucket> new_bucket = std::make_shared<Bucket>(bucket_size_, ptr->GetDepth());
            dir_[index + cap] = new_bucket;
            for (size_t i = 0; i < bucket_size_; i++) {
                auto kv = ptr->GetItems().front();
                ptr->GetItems().pop_front();
                size_t idx = IndexOf(kv.first);
                dir_[idx]->GetItems().push_back(kv);
            }

        }
        else {//L<G
            ptr->IncrementDepth();
            size_t cap = dir_.size() / 2;
            //redistribute
            std::shared_ptr<Bucket> new_bucket = std::make_shared<Bucket>(bucket_size_, ptr->GetDepth());
            for (size_t i = 0; i < bucket_size_ / 2; i++) {
                auto kv = ptr->GetItems().front();
                ptr->GetItems().pop_front();
                new_bucket->GetItems().push_back(kv);
            }
            dir_[(index + cap)%dir_.size()] = new_bucket;
        }
        //add kv
        index = IndexOf(key);
        ptr = dir_[index];
        ptr->Insert(key, value);
    }
}

//===--------------------------------------------------------------------===//
// Bucket
//===--------------------------------------------------------------------===//
template <typename K, typename V>
ExtendibleHashTable<K, V>::Bucket::Bucket(size_t array_size, int depth) : size_(array_size), depth_(depth) {}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Find(const K& key, V& value) -> bool {
    auto it = find_if(list_.begin(), list_.end(), [key](std::pair<K, V> kv)->bool { return kv.first == key; });
    if (it == list_.end()) {
        return false;
    }
    else {
        value = it->second;
        return true;
    }
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Remove(const K& key) -> bool {
    auto it = find_if(list_.begin(), list_.end(), [key](std::pair<K, V> kv)->bool { return kv.first == key; });
    if (it == list_.end()) {
        return false;
    }
    else {
        list_.erase(it);
        return true;
    }
}

template <typename K, typename V>
auto ExtendibleHashTable<K, V>::Bucket::Insert(const K& key, const V& value) -> bool {
    auto it = find_if(list_.begin(), list_.end(), [key](std::pair<K, V> kv)->bool { return kv.first == key; });
    if (it == list_.end()) {
        if (IsFull()) {
            return false;
        }
        list_.push_front({ key , value });
        return true;
    }
    else {
        it->second = value;
        return true;
    }
}

template class ExtendibleHashTable<page_id_t, Page *>;
template class ExtendibleHashTable<Page *, std::list<Page *>::iterator>;
template class ExtendibleHashTable<int, int>;
// test purpose
template class ExtendibleHashTable<int, std::string>;
template class ExtendibleHashTable<int, std::list<int>::iterator>;

}  // namespace bustub
