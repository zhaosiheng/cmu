//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/page/b_plus_tree_internal_page.cpp
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <sstream>

#include "common/exception.h"
#include "storage/page/b_plus_tree_internal_page.h"
#include "storage/index/b_plus_tree.h"
namespace bustub {


/*****************************************************************************
 * HELPER METHODS AND UTILITIES
 *****************************************************************************/
/*
 * Init method after creating a new internal page
 * Including set page type, set current size, set page id, set parent id and set
 * max page size
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(page_id_t page_id, page_id_t parent_id, int max_size) {
  SetParentPageId(parent_id);
  SetPageId(page_id);
  SetMaxSize(max_size);
  SetPageType(IndexPageType::INTERNAL_PAGE);

  SetSize(0);
}

/*
 * Helper method to get/set the key associated with input "index"(a.k.a
 * array offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType {
  if(index < GetSize()){
    return array_[index].first;
  }
  KeyType key{};
  return key;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) {
  if(index < GetSize()){
    array_[index].first = key;
  }
}

/*
 * Helper method to get the value associated with input "index"(a.k.a array
 * offset)
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType {
  if(index < GetSize()){
    return array_[index].second;
  }
  return 0;
}
INDEX_TEMPLATE_ARGUMENTS
template<typename mValueType>
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::insert_key(const KeyType &key, const ValueType &value, const KeyComparator &comparator, BPlusTree<KeyType, mValueType, KeyComparator>* tree){
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
    BPlusTreePage* page = tree->pid_to_page(GetParentPageId());
    typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage *parent;
    if(page){/*has parent*/
      parent = reinterpret_cast<typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage*>(page);
    }else{/*no parent*/
      page_id_t tmp;
      parent = tree->new_internal_page(tmp);
      tree->update_root(tmp);
      SetParentPageId(tmp);
    }
    /*new_internal, redistribute, parent+1*/
    //new_internal
    page_id_t nid;
    auto next_page = tree->new_internal_page(nid, GetParentPageId());
    //redistribute
    for(int i=0;i<GetMinSize();i++){
      next_page->insert_key(array_[GetSize()-1].first, array_[GetSize()-1].second, comparator, tree);
      IncreaseSize(-1);
    }
    //parent+1: parent will judge wheather it need to split
    parent->insert_key(next_page->KeyAt(0), nid, comparator, tree);
        
  }
  return;
}
#define SOLVE(x) BPlusTreeInternalPage<GenericKey<x>, page_id_t, GenericComparator<x>>::insert_key(const GenericKey<x>& key, const page_id_t &value,  GenericComparator<x> &comparator,BPlusTree<GenericKey<x>, RID, GenericComparator<x>>* tree)
template void SOLVE(4);
template void SOLVE(8);
template void SOLVE(16);
template void SOLVE(32);
template void SOLVE(64);
// valuetype for internalNode should be page id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t, GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t, GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t, GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t, GenericComparator<64>>;
}  // namespace bustub
