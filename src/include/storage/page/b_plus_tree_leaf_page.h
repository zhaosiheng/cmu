//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/include/page/b_plus_tree_leaf_page.h
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#pragma once

#include <utility>
#include <vector>

#include "storage/page/b_plus_tree_page.h"
#include "common/logger.h"
namespace bustub {
template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTree;
#define B_PLUS_TREE_LEAF_PAGE_TYPE BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>
#define LEAF_PAGE_HEADER_SIZE 28
#define LEAF_PAGE_SIZE ((BUSTUB_PAGE_SIZE - LEAF_PAGE_HEADER_SIZE) / sizeof(MappingType))

/**
 * Store indexed key and record id(record id = page id combined with slot id,
 * see include/common/rid.h for detailed implementation) together within leaf
 * page. Only support unique key.
 *
 * Leaf page format (keys are stored in order):
 *  ----------------------------------------------------------------------
 * | HEADER | KEY(1) + RID(1) | KEY(2) + RID(2) | ... | KEY(n) + RID(n)
 *  ----------------------------------------------------------------------
 *
 *  Header format (size in byte, 28 bytes in total):
 *  ---------------------------------------------------------------------
 * | PageType (4) | LSN (4) | CurrentSize (4) | MaxSize (4) |
 *  ---------------------------------------------------------------------
 *  -----------------------------------------------
 * | ParentPageId (4) | PageId (4) | NextPageId (4)
 *  -----------------------------------------------
 */
INDEX_TEMPLATE_ARGUMENTS
class BPlusTreeLeafPage : public BPlusTreePage {
 public:
  // After creating a new leaf page from buffer pool, must call initialize
  // method to set default values
  void Init(page_id_t page_id, page_id_t parent_id = INVALID_PAGE_ID, int max_size = LEAF_PAGE_SIZE);
  // helper methods
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index) const -> KeyType;
  auto ValueAt(int index) const -> ValueType;
  //mine
  bool k_to_v(const KeyType &key,ValueType &value, const KeyComparator &comparator){
    for(int i=0;i<GetSize();i++){
      int rs = comparator(KeyAt(i), key);
      if(rs == 0){
        value = array_[i].second;
        return true;
      }
    }
    return false;
  }
  bool insert(const KeyType &key, const ValueType &value, const KeyComparator &comparator, BPlusTree<KeyType, ValueType, KeyComparator>* tree);
  /*insert in order. will appear in for{}*/
  void batch_insert(const KeyType &key, const ValueType &value){
    array_[GetSize()].first = key;
    array_[GetSize()].second = value;
    IncreaseSize(1);
    LOG_DEBUG("# add a kv in leaf=%d, cur_num=%d", GetPageId(), GetSize());
  }
  
  void remove(const KeyType &key, const KeyComparator &comparator, BPlusTree<KeyType, ValueType, KeyComparator>* tree){
    int pos = -1;
    for(int i=0;i<GetSize();i++){
      int rs = comparator(KeyAt(i), key);
      if(rs == 0){
        pos = i;
      }
    }
    if(pos == -1)/*not find*/
      return;
    /*remove*/  
    IncreaseSize(-1);
    for(int i=pos;i<GetSize();i++){
      array_[i] = array_[i+1];
    }
    //need lend
    if(GetSize()<GetMinSize()){
      if(GetParentPageId() == INVALID_PAGE_ID) return;
      typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage *parent;
      parent = reinterpret_cast<typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage*>(tree->pid_to_page(GetParentPageId()));
      if(parent->GetSize() == 1) return;
      page_id_t bro_id = parent->get_sibling(GetPageId());
      typename BPlusTree<KeyType, ValueType, KeyComparator>::LeafPage *bro;
      bro = reinterpret_cast<typename BPlusTree<KeyType, ValueType, KeyComparator>::LeafPage*>(tree->pid_to_page(bro_id));
      //merge:cur->->->bro
      if(bro->GetSize() + GetSize() <= GetMaxSize()){
        int size = GetSize();
        for(int i=0;i<size;i++){
          IncreaseSize(-1);
          bro->insert(KeyAt(i), ValueAt(i), comparator, tree);
        }
        parent->remove(GetPageId(), comparator, tree);
      }else{//lend one kv
        int pos = bro->GetSize()-1;
        insert(bro->KeyAt(pos), bro->ValueAt(pos), comparator, tree);
        bro->IncreaseSize(-1);
      }
      
    }
  }
 private:
  page_id_t next_page_id_;
  // Flexible array member for page data.
  MappingType array_[1];
};
}  // namespace bustub
