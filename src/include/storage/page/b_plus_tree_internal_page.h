//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/include/page/b_plus_tree_internal_page.h
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#pragma once

#include <queue>

#include "storage/page/b_plus_tree_page.h"

namespace bustub {

#define B_PLUS_TREE_INTERNAL_PAGE_TYPE BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>
#define INTERNAL_PAGE_HEADER_SIZE 24
#define INTERNAL_PAGE_SIZE ((BUSTUB_PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / (sizeof(MappingType)))
/**
 * Store n indexed keys and n+1 child pointers (page_id) within internal page.
 * Pointer PAGE_ID(i) points to a subtree in which all keys K satisfy:
 * K(i) <= K < K(i+1).
 * NOTE: since the number of keys does not equal to number of child pointers,
 * the first key always remains invalid. That is to say, any search/lookup
 * should ignore the first key.
 *
 * Internal page format (keys are stored in increasing order):
 *  --------------------------------------------------------------------------
 * | HEADER | KEY(1)+PAGE_ID(1) | KEY(2)+PAGE_ID(2) | ... | KEY(n)+PAGE_ID(n) |
 *  --------------------------------------------------------------------------
 */
INDEX_TEMPLATE_ARGUMENTS
class BPlusTreeInternalPage : public BPlusTreePage {
 public:
  // must call initialize method after "create" a new node
  void Init(page_id_t page_id, page_id_t parent_id = INVALID_PAGE_ID, int max_size = INTERNAL_PAGE_SIZE);

  auto KeyAt(int index) const -> KeyType;
  void SetKeyAt(int index, const KeyType &key);
  auto ValueAt(int index) const -> ValueType;
  //mine
  KeyType lookup(const KeyType &key, const KeyComparator &comparator){
    /*array_[0]_is_invalid*/
    for(int i=1;i<GetSize();i++){
      int rs = comparator(KeyAt(i), key);
      if(rs == -1){
        return ValueAt(i-1);
      }else if(rs == 0){
        return ValueAt(i);
      }else if(rs == 1){
        continue;
      }
      return ValueAt(GetSize() - 1);
    }
  }
  void insert_key(const KeyType &key, ValueType &value, const KeyComparator &comparator, BufferPoolManager* bgm, std::string name){
    int pos;/*where need to insert*/
    /*array_[0]_is_invalid*/
    for(int i=1;i<GetSize();i++){
      int rs = comparator(KeyAt(i), key);
      if(rs == 1){
        pos =i;
        break;
      }
    }
    /*m_size+1*/
    IncreaseSize(1);
    for(int i=GetSize()-1;i>pos;i--){
      array_[i]=array_[i-1];
    }
    array_[pos].first = key;
    array_[pos].second = value;

    if(GetSize()>GetMaxSize()){/*out of maxsize*/
      BPlusTreePage* page = pid_to_page(GetParentPageId(), bgm);
      BPLUSTREE_TYPE::InternalPage *parent;
      if(page){/*has parent*/
        parent = reinterpret_cast<BPLUSTREE_TYPE::InternalPage*>(page);
      }else{/*no parent*/
        parent = new_internal_page(name, bpm);
      }
      /*new_internal, redistribute, parent+1*/
      //new_internal
      auto next_page = new_internal_page(name, bpm, GetParentPageId());
      //redistribute
      for(int i=0;i<GetMinSize();i++){
        next_page->insert_key(array_[GetSize()-1].first, array_[GetSize()-1].second, comparator, bpm);
        IncreaseSize(-1);
      }
      //parent+1: parent will judge wheather it need to split
      parent->insert_key(next_page->KeyAt(0), next_page_id_, comparator, bpm);
          
    }
    return;
  }

 private:
  // Flexible array member for page data.
  MappingType array_[1];
};
}  // namespace bustub
