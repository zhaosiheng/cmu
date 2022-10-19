//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_leaf_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <sstream>

#include "common/exception.h"
#include "common/rid.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/index/b_plus_tree.h"
#include "storage/page/b_plus_tree_internal_page.h"
namespace bustub {

/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/

/**
 * Init method after creating a new leaf page
 * Including set page type, set current size to zero, set page id/parent id, set
 * next page id and set max size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id, int max_size) {
  SetPageId(page_id);
  SetParentPageId(parent_id);
  SetMaxSize(max_size);
  SetPageType(IndexPageType::LEAF_PAGE);

  SetSize(0);
}

/**
 * Helper methods to set/get next page id
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t { return next_page_id_; }

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) {next_page_id_ = next_page_id;}

/*
 * Helper method to find and return the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  if(index < GetSize()){
    return array_[index].first;
  }  
  KeyType key{};
  return key;
}
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValueAt(int index) const -> ValueType {
  if(index < GetSize()){
    return array_[index].second;
  }
  return 0;
}

/*if key already exist, return false*/
INDEX_TEMPLATE_ARGUMENTS
bool B_PLUS_TREE_LEAF_PAGE_TYPE::insert(const KeyType &key, const ValueType &value, const KeyComparator &comparator, BPlusTree<KeyType, ValueType, KeyComparator>* tree){
  int pos = 0;//where need to insert
  for(int i=0;i<GetSize();i++){
    int rs = comparator(KeyAt(i), key);
    if(rs == 0){
      return false;
    }else if(rs == 1){
      pos = i;
      break;
    }
  }
  /*m_size+1*/
  IncreaseSize(1);
  for(int i=GetSize()-1;i>pos;i--){
    array_[i]=array_[i-1];
  }
  /*update_parent's_k*/
  if(pos == 0 && GetSize()<=GetMaxSize() && GetParentPageId()!=INVALID_PAGE_ID){
    BPlusTreePage* page = tree->pid_to_page(GetParentPageId());
    typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage *parent;
    parent = reinterpret_cast<typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage*>(page);
    parent->update_value(KeyAt(0), key);
  }
  array_[pos].first = key;
  array_[pos].second = value;

  if(GetSize() > GetMaxSize()){/*out of maxsize*/
    BPlusTreePage* page = tree->pid_to_page(GetParentPageId());
    typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage *parent;
    if(page){/*has parent*/
      parent = reinterpret_cast<typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage*>(page);
    }else{/*no parent*/
      page_id_t tmp;
      parent = tree->new_internal_page(tmp);
      tree->update_root(tmp);
      SetParentPageId(tmp);
    }
    /*new_leaf, redistribute, parent+1*/
    //new_leaf
    page_id_t nid;
    auto next_page = tree->new_leaf_page(nid, GetNextPageId(), GetParentPageId());
    SetNextPageId(nid);
    //redistribute
    int start = GetSize() - 1 - GetMinSize();
    for(int i=0;i<GetMinSize();i++){
      next_page->batch_insert(array_[start].first, array_[start].second);
      start++;
      IncreaseSize(-1);
    }
    //parent+1: parent will judge wheather it need to split
    parent->insert_key(next_page->KeyAt(0), nid, comparator, tree, KeyAt(0), GetPageId());
  }
  return true;
}

template class BPlusTreeLeafPage<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTreeLeafPage<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTreeLeafPage<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTreeLeafPage<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
