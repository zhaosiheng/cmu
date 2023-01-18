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

#include "common/logger.h"
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
  void Init(page_id_t page_id, page_id_t parent_id = INVALID_PAGE_ID, int max_size = INTERNAL_PAGE_SIZE, BufferPoolManager *manager = nullptr);

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
      }
    }
    return ValueAt(GetSize() - 1);
  }
  /*x->x+1->...->n->n-1*/
  ValueType get_sibling(const ValueType &value){
    int cur = GetSize() - 1;//size>=2
    /*array_[0]_is_invalid*/
    for(int i=1;i<GetSize();i++){
      if(ValueAt(i) == value){
        cur = i;
      }
    }
    if(cur == GetSize() - 1) return ValueAt(GetSize() - 2);
    else return ValueAt(cur + 1);
  }
  void update_key(const KeyType &tar, const ValueType &val){
    for(int i=0;i<GetSize();i++){
      if(ValueAt(i) == val){
        array_[i].first = tar;
        return;
      }
    }
  }
  /*insert in order. will appear in for{}*/
  void batch_insert(const KeyType &key, const ValueType &value){
    array_[GetSize()].first = key;
    array_[GetSize()].second = value;
    IncreaseSize(1);
    LOG_DEBUG("# add a kv in internal=%d, cur_num=%d", GetPageId(), GetSize());
  }
  template<typename mValueType>
  void _insert_key(const KeyType &key, const ValueType &value, const KeyComparator &comparator, BPlusTree<KeyType, mValueType, KeyComparator>* tree){
    int pos = 0;//where need to insert
    for(;pos<GetSize();pos++){
      int rs = comparator(KeyAt(pos), key);
      if(rs == 0){
        return;
      }else if(rs == 1){
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
    LOG_DEBUG("# add a kv in internal=%d, cur_num=%d", GetPageId(), GetSize());

    /*update_parent's_k, happened in removing*/
    if(pos == 0 && GetSize() == GetMinSize()){
      if(GetParentPageId() == INVALID_PAGE_ID) return;
      typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage *parent;
      parent = reinterpret_cast<typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage*>(tree->pid_to_page(GetParentPageId()));
      parent->update_key(KeyAt(0), GetPageId());
      return;
    }

    if(GetSize() > GetMaxSize()){/*out of maxsize*/
      LOG_DEBUG("# internal=%d need to split", GetPageId());
      BPlusTreePage* page = tree->pid_to_page(GetParentPageId());
      typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage *parent;
      if(page){/*has parent*/
        LOG_DEBUG("# internal=%d has parent=%d", GetPageId(), GetParentPageId());
        parent = reinterpret_cast<typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage*>(page);
      }else{/*no parent*/
        LOG_DEBUG("# internal=%d has no parent", GetPageId());
        page_id_t tmp;
        parent = tree->new_internal_page(tmp);
        LOG_DEBUG("# new internal=%d", parent->GetPageId());
        tree->Update_root(tmp);
        SetParentPageId(tmp);
        LOG_DEBUG("# add an internal page=%d as leaf=%d's parent", GetParentPageId(), GetPageId());
        parent->_insert_key(KeyAt(0), GetPageId(), comparator, tree);
      }
      /*new_internal, redistribute, parent+1*/
      //new_internal
      page_id_t nid;
      auto next_page = tree->new_internal_page(nid, GetParentPageId());
      LOG_DEBUG("# new internal=%d", next_page->GetPageId());
      //redistribute
      int start = GetSize() - GetMinSize();
      for(int i=0;i<GetMinSize();i++){
        next_page->batch_insert(array_[start].first, array_[start].second);
        start++;
        //need to update child's parent
        IncreaseSize(-1);
        LOG_DEBUG("# rm a kv from internal=%d, cur_num=%d", GetPageId(), GetSize());
      }
      LOG_DEBUG("# finish redistribution");
      //parent+1: parent will judge wheather it need to split
      parent->_insert_key(next_page->KeyAt(0), next_page->GetPageId(), comparator, tree);
    }
  }
 
  template<typename mValueType>
  void remove(const ValueType &val, const KeyComparator &comparator, BPlusTree<KeyType, mValueType, KeyComparator>* tree){
    int pos = -1;
    for(int i=0;i<GetSize();i++){
      if(ValueAt(i) == val){
        pos = i;
      }
    }
    if(pos == -1)/*not find*/
      return;
    BPlusTreePage* pos_page = tree->pid_to_page(ValueAt(pos));
    if(pos_page->IsLeafPage()){
      int prev = pos - 1;

      if(prev >= 0) tree->set_leaf_next(ValueAt(prev), ValueAt(pos));
    }
    /*remove*/  
    IncreaseSize(-1);
    for(int i=pos;i<GetSize() - 1;i++){
      array_[i] = array_[i+1];
    }
    //need lend or merge
    if(GetSize() <= GetMinSize()){
      if(GetParentPageId() == INVALID_PAGE_ID){
        if(GetSize() == 1){//delete page
          buffer_pool_manager_->DeletePage(GetPageId());
          SetPageId(INVALID_PAGE_ID);
          tree->Update_root(ValueAt(0));
          return;
        }else{//maintain cur stats
          return;
        }
      }
      typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage *parent;
      parent = reinterpret_cast<typename BPlusTree<KeyType, mValueType, KeyComparator>::InternalPage*>(tree->pid_to_page(GetParentPageId()));
      if(parent->GetSize() == 1) return;
      page_id_t bro_id = parent->get_sibling(GetPageId());
      typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage *bro;
      bro = reinterpret_cast<typename BPlusTree<KeyType, ValueType, KeyComparator>::InternalPage*>(tree->pid_to_page(bro_id));
      //merge:cur<<<-bro
      if(bro->GetSize() + GetSize() <= GetMaxSize()){
        int size = bro->GetSize();
        for(int i=0;i<size;i++){
          bro->IncreaseSize(-1);
          _insert_key(bro->KeyAt(i), bro->ValueAt(i), comparator, tree);
        }
        buffer_pool_manager_->DeletePage(bro->GetPageId());
        parent->remove(bro->GetPageId(), comparator, tree);
      }else{//lend:cur<<<-bro
        int cmp = comparator(KeyAt(0), bro->KeyAt(0));// cur>bro == 1
        int pos = cmp > 0 ? bro->GetSize() - 1 : 0;
        _insert_key(bro->KeyAt(pos), bro->ValueAt(pos), comparator, tree);
        bro->remove(bro->ValueAt(pos), comparator, tree);
      }
    }
  }
 private:
 //using to update child's parent
  BufferPoolManager *buffer_pool_manager_;
  // Flexible array member for page data.
  MappingType array_[1];
};
}  // namespace bustub
