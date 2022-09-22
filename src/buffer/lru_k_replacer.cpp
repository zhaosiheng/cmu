//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"

namespace bustub {

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {
    lst = new my_list(num_frames,k);
}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
    std::scoped_lock sl(latch_);
    return lst->evcit(frame_id);
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
   std::scoped_lock sl(latch_);
   lst->add(frame_id);
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    std::scoped_lock sl(latch_);
    lst->set(frame_id,set_evictable);
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
    std::scoped_lock sl(latch_);
    lst->remove(frame_id);
}

auto LRUKReplacer::Size() -> size_t { 
    std::scoped_lock sl(latch_);
    return lst->get_cur();
}

}  // namespace bustub
