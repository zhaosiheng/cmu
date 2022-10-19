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
//#include "storage/index/b_plus_tree.h"
namespace bustub {
template <typename KeyType, typename ValueType, typename KeyComparator>
class BPlusTree;

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
  ValueType lookup(const KeyType &key, const KeyComparator &comparator){
    /*array_[0]_is_invalid*/
    for(int i=1;i<GetSize();i++){
      int rs = comparator(KeyAt(i), key);
      if(rs == 1){
        return ValueAt(i-1);
      }else if(rs == 0){
        return ValueAt(i);
      }else if(rs == -1){
        continue;
      }
    }
    return ValueAt(GetSize() - 1);
  }
  ValueType get_sibling(const ValueType &value){
    int cur = GetSize() - 1;//cur_pos
    /*array_[0]_is_invalid*/
    for(int i=1;i<GetSize();i++){
      if(ValueAt(i) == value){
        cur = i;
      }
    }
    int tar;
    if(cur == 0){
      tar = cur + 1;
    }else if(cur == GetSize() - 1){
      tar = cur - 1;
    }else{
      tar = cur + 1;
    }
    return ValueAt(tar);
  }
  void update_value(const KeyType &src, const ValueType &tar, const KeyComparator &comparator){
    for(int i=0;i<GetSize();i++){
      int rs = comparator(KeyAt(i), src);
      if(rs == 0){
        array_[i].second = tar;
        return;
      }
    }
  }
  /*insert in order. will appear in for{}*/
  void batch_insert(const KeyType &key, const ValueType &value){
    array_[GetSize()].first = key;
    array_[GetSize()].second = value;
    IncreaseSize(1);
  }
  template<typename mValueType>
  void insert_key(const KeyType &key, const ValueType &value, const KeyComparator &comparator, BPlusTree<KeyType, mValueType, KeyComparator>* tree, const KeyType &l_key = {}, const ValueType &l_value = 0){
    if(GetSize()==0 && l_value!=0){
      IncreaseSize(2);
      array_[GetSize()-1].first = key;
      array_[GetSize()-1].second = value;

      array_[0].first = l_key;
      array_[0].second = l_value;     
      return; 
    }
    int pos = 0 ;/*where need to insert*/
    /*array_[0]_is_invalid*/
    for(int i=1;i<GetSize();i++){
      int rs = comparator(KeyAt(i), l_key);
      if(rs == 0){
        pos = i;
        break;
      }else if(rs == 1){
        pos = i-1;
        break;
      }
    }
    /*m_size+1*/
    IncreaseSize(1);
    for(int i=GetSize()-1;i>pos+1;i--){
      array_[i]=array_[i-1];
    }
    array_[pos+1].first = key;
    array_[pos+1].second = value;

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
      int start = GetSize() - 1 - GetMinSize();
      for(int i=0;i<GetMinSize();i++){
        next_page->batch_insert(array_[start].first, array_[start].second);
        start++;
        IncreaseSize(-1);
      }
      //parent+1: parent will judge wheather it need to split
      parent->insert_key(next_page->KeyAt(0), nid, comparator, tree, KeyAt(0), GetPageId());
          
    }
    return;    
  }
  template<typename mValueType>
  void remove(const ValueType &val, const KeyComparator &comparator, BPlusTree<KeyType, mValueType, KeyComparator>* tree){
    int pos = 0;
    for(int i=0;i<GetSize();i++){
      if(ValueAt(i) == val){
        pos = i;
      }
    }
    for(int i=GetSize()-1;i>=pos+1;i--){
      array_[i-1] = array_[i];
    }
    IncreaseSize(-1);
    if(GetSize()<GetMinSize()){
      if(GetParentPageId() == INVALID_PAGE_ID) return;
      typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage *parent;
      parent = reinterpret_cast<typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage*>(tree->pid_to_page(GetParentPageId()));
      if(parent->GetSize() == 1) return;
      page_id_t bro_id = parent->get_sibling(GetPageId());
      typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage *bro;
      bro = reinterpret_cast<typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage*>(tree->pid_to_page(bro_id));
      //merge:cur->->->bro
      if(bro->GetSize() + GetSize() <= GetMaxSize()){
        int size = GetSize();
        for(int i=0;i<size;i++){
          IncreaseSize(-1);
          bro->insert_key(KeyAt(i), ValueAt(i), comparator, tree);
        }
        parent->remove(GetPageId(), comparator, tree);
      }else{//lend one kv
        int pos = bro->GetSize()-1;
        insert_key(bro->KeyAt(pos), bro->ValueAt(pos), comparator, tree);
        bro->IncreaseSize(-1);
      }
    }
  }
 private:
  // Flexible array member for page data.
  MappingType array_[1];
};
}  // namespace bustub
