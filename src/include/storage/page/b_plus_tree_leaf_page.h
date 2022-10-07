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
#include "storage/index/b_plus_tree.h"
namespace bustub {

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
  //mine
  bool lookup(const KeyType &key,ValueType &value, const KeyComparator &comparator){
    for(int i=0;i<GetSize();i++){
      int rs = comparator(KeyAt(i), key);
      if(rs == 0){
        value = array_[i].second;
        return true;
      }
    }
    return false;
  }
  /*if key already exist, return false*/
  bool insert(const KeyType &key, const ValueType &value, const KeyComparator &comparator, BPLUSTREE_TYPE* tree){
    int pos;//where need to insert
    for(int i=0;i<GetSize();i++){
      int rs = comparator(KeyAt(i), key);
      if(rs == 0){
        return false;
      }else if(rs == -1){
        pos = i;
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

    if(GetSize() > GetMaxSize()){/*out of maxsize*/
      BPlusTreePage* page = tree->pid_to_page(GetParentPageId());
      BPLUSTREE_TYPE::InternalPage *parent;
      if(page){/*has parent*/
        parent = reinterpret_cast<BPLUSTREE_TYPE::InternalPage*>(page);
      }else{/*no parent*/
        page_id_t tmp;
        parent = tree->new_internal_page(&tmp);
        tree->UpdateRootPageId(tmp);
        SetParentPageId(tmp);
      }
      /*new_leaf, redistribute, parent+1*/
      //new_leaf
      page_id_t nid;
      auto next_page = tree->new_leaf_page(&nid, GetNextPageId(), GetParentPageId());
      SetNextPageId(nid);
      //redistribute
      for(int i=0;i<GetMinSize();i++){
        next_page->insert(array_[GetSize()-1].first, array_[GetSize()-1].second, bpm);
        IncreaseSize(-1);
      }
      //parent+1: parent will judge wheather it need to split
      parent->insert_key(next_page->KeyAt(0), nid, comparator);
    }
    return true;
  }
 private:
  page_id_t next_page_id_;
  // Flexible array member for page data.
  MappingType array_[1];
};
}  // namespace bustub
