//===----------------------------------------------------------------------===//
//
//                         CMU-DB Project (15-445/645)
//                         ***DO NO SHARE PUBLICLY***
//
// Identification: src/include/index/index_iterator.h
//
// Copyright (c) 2018, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
/**
 * index_iterator.h
 * For range scan of b+ tree
 */
#pragma once
#include "storage/page/b_plus_tree_leaf_page.h"

namespace bustub {

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

INDEX_TEMPLATE_ARGUMENTS
class IndexIterator {
 public:
  // you may define your own constructor based on your member variables
  IndexIterator(BPlusTree<KeyType, ValueType, KeyComparator>* t){
    this->tree = t;
    this->buffer_pool_manager_ = t->get_buffer();
    if(t->IsEmpty()){
      this->leaf_page = nullptr;
      this->pt = 0;
    }else{

    }

  }
  ~IndexIterator(){}  // NOLINT

  auto IsEnd() -> bool{
    return pt->GetNextPageId() == INVALID_PAGE_ID;
  }

  auto operator*() -> const MappingType &{
    return pt->
  }

  auto operator++() -> IndexIterator &{
    
  }

  auto operator==(const IndexIterator &itr) const -> bool { throw std::runtime_error("unimplemented"); }

  auto operator!=(const IndexIterator &itr) const -> bool { throw std::runtime_error("unimplemented"); }

 private:
  // add your own private member variables here
  BPlusTree<KeyType, ValueType, KeyComparator> *tree;
  BufferPoolManager *buffer_pool_manager_;
  BPlusTreeLeafPage<KeyType, ValueType, KeyComparator> *leaf;
  int pt;
};

}  // namespace bustub
