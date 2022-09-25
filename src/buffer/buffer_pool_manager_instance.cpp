//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager_instance.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2021, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager_instance.h"

#include "common/macros.h"

namespace bustub {

void pin(page_id_t pid){
  frame_id_t fid;
  page_table_->Find(pid,fid);
  Page *p = &pages_[fid];
  p->GetPinCount()++;
  replacer_->SetEvictable(fid,false);
}

BufferPoolManagerInstance::BufferPoolManagerInstance(size_t pool_size, DiskManager *disk_manager, size_t replacer_k,
                                                     LogManager *log_manager)
    : pool_size_(pool_size), disk_manager_(disk_manager), log_manager_(log_manager) {
  // we allocate a consecutive memory space for the buffer pool
  pages_ = new Page[pool_size_];
  page_table_ = new ExtendibleHashTable<page_id_t, frame_id_t>(bucket_size_);
  replacer_ = new LRUKReplacer(pool_size, replacer_k);

  // Initially, every page is in the free list.
  for (size_t i = 0; i < pool_size_; ++i) {
    free_list_.emplace_back(static_cast<int>(i));
  }
}

BufferPoolManagerInstance::~BufferPoolManagerInstance() {
  delete[] pages_;
  delete page_table_;
  delete replacer_;
}

auto BufferPoolManagerInstance::NewPgImp(page_id_t *page_id) -> Page * {
  std::scoped_lock lock{latch_};
  frame_id_t &fid;
  if(!free_list_.empty()){
    fid = free_list_.front();
    free_list_.pop_front();
  }else if(!replacer_->Evict(fid)){
    return nullptr;
  }

  Page *p = &pages_[fid];
  if(p->IsDirty()){
    disk_manager_->WritePage(p->page_id_, p->GetData());
  }
  page_table_->Remove(p->GetPageId());
  //reset
  *page_id = AllocatePage();
  p->GetPinCount() = 1;
  p->GetPageId() = *page_id;
  p->IsDirty() = false;
  page_table_->insert(*page_id , fid);
  replacer_->RecordAccess(fid);
  replacer_->SetEvictable(fid , false);
  //???
  disk_manager_->WritePage(p->page_id_, p->GetData());
  return p;
}

auto BufferPoolManagerInstance::FetchPgImp(page_id_t pid) -> Page * {
  std::scoped_lock lock{latch_};
  frame_id_t &fid;
  if(page_table_->Find(pid , fid)){
    replacer_->RecordAccess(fid);
    replacer_->SetEvictable(fid , false);
    pages_[fid].GetPinCount()++;
    return &pages_[fid];
  }
  //
  if(!free_list_.empty()){
    fid = free_list_.front();
    free_list_.pop_front();
  }else if(!replacer_->Evict(fid)){
    return nullptr;
  }
  Page *p = &pages_[fid];
  p->GetPinCount() = 1;
  p->GetPageId() = pid;
  p->IsDirty() = false;
  disk_manager_->ReadPage(pid, p->GetData());
  replacer_->RecordAccess(fid);
  replacer_->SetEvictable(fid , false);
  return p;
}

auto BufferPoolManagerInstance::UnpinPgImp(page_id_t page_id, bool is_dirty) -> bool {
  std::scoped_lock lock{latch_};
  frame_id_t &fid;
  if(!page_table_->Find(page_id,fid)){
    return false;
  }
  Page *p = &pages_[fid];
  if(p->GetPinCount() <= 0){
    return false;
  }
  if(is_dirty){
    p->IsDirty() = true;
  }
  if(--(p->GetPinCount()) == 0){
    replacer_->SetEvictable(fid , true);
  }
  return true;
}

auto BufferPoolManagerInstance::FlushPgImp(page_id_t page_id) -> bool {
  std::scoped_lock lock{latch_};
  frame_id_t &fid;
  if(!page_table_->Find(page_id,fid) || page_id == INVALID_PAGE_ID){
    return false;
  }
  Page *p = &pages_[fid];
  disk_manager_->WritePage(page_id, page->GetData());
  p->IsDirty() = false;
  return true;
}

void BufferPoolManagerInstance::FlushAllPgsImp() {
  std::scoped_lock lock{latch_};
  for(size_t i=0;i<pool_size;i++){
    disk_manager_->WritePage(pages_[i] ,pages_[i].GetData());
  }
}

auto BufferPoolManagerInstance::DeletePgImp(page_id_t page_id) -> bool {
  std::scoped_lock lock{latch_};
  frame_id_t &fid;
  if(!page_table_->Find(page_id,fid)){
    return true;
  }
  Page *p = &pages_[fid];
  if(p->GetPinCount() > 0){
    return false;
  }
  if(p->IsDirty()){
    disk_manager_->WritePage(page_id, page->GetData());
  }
  disk_manager_->DeallocatePage(page_id);
  page_table_->Remove(page_id);
  p->page_id_ = INVALID_PAGE_ID;
  p->pin_count_ = 0;
  p->is_dirty_ = false;
  free_list_.push_back(fid);
  return true;
}

auto BufferPoolManagerInstance::AllocatePage() -> page_id_t { return next_page_id_++; }

}  // namespace bustub
