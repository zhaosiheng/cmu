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

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool { return false; }

void LRUKReplacer::RecordAccess(frame_id_t frame_id) {
    my_buffer_.push_back({frame_id , true});
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
    //find in buffer
    auto it = find_if(my_buffer_.begin() , my_buffer_.end() , [frame_id](std::pair<frame_id_t , bool> tm)->bool{ return tm.first == frame_id;});
    if(it == my_buffer_.end()){
        //find in buffer_k
        it = find_if(my_buffer_k_.begin() , my_buffer_k_.end() , [frame_id](std::pair<frame_id_t , bool> tm)->bool{ return tm.first == frame_id;});
        if(it == my_buffer_.end()){
            std::cout<<"do not find frame_id";
            std::abort();
        }
    }
    if(it->second && !set_evictable){
        curr_size_--;
    }else if(!it->second && set_evictable){
        curr_size_++;
    }
    it->second = set_evictable;
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
    //find in buffer
    auto it = find_if(my_buffer_.begin() , my_buffer_.end() , [frame_id](std::pair<frame_id_t , bool> tm)->bool{ return tm.first == frame_id;});
    if(it == my_buffer_.end()){
        //find in buffer_k
        it = find_if(my_buffer_k_.begin() , my_buffer_k_.end() , [frame_id](std::pair<frame_id_t , bool> tm)->bool{ return tm.first == frame_id;});
        if(it == my_buffer_.end()) return;
    }
    if(it->second){
        curr_size_--;
    }
    my_buffer_.erase(it);
    current_timestamp_--;
}

auto LRUKReplacer::Size() -> size_t { return curr_size_; }

}  // namespace bustub
