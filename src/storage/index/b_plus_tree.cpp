#include <string>

#include "common/exception.h"
#include "common/logger.h"
#include "common/rid.h"
#include "storage/index/b_plus_tree.h"
#include "storage/page/header_page.h"
#include <map>
#include "common/logger.h"
namespace bustub {
/*global_config_map*/
INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, BufferPoolManager *buffer_pool_manager, const KeyComparator &comparator,
                          int leaf_max_size, int internal_max_size)
    : index_name_(std::move(name)),
      root_page_id_(INVALID_PAGE_ID),
      buffer_pool_manager_(buffer_pool_manager),
      comparator_(comparator),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size) {}

/*
 * Helper function to decide whether current b+tree is empty
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  return root_page_id_ == INVALID_PAGE_ID;
}
/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/*
 * Return the only value that associated with input key
 * This method is used for point query
 * @return : true means key exists
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result, Transaction *transaction) -> bool {
  if(IsEmpty()){
    return false;
  }
  Page *page;
  /*internal->...->internal->*/
  //fetch
  if(!(page = buffer_pool_manager_->FetchPage(root_page_id_))){
    return false;
  }
  BPlusTreePage *t_page = reinterpret_cast<BPlusTreePage*>(page->GetData());
  buffer_pool_manager_->UnpinPage(root_page_id_, false);
  //unpin
  while(t_page->IsRootPage()){
    LOG_DEBUG("# find a internal=%d", t_page->GetPageId());
    auto cur_page = reinterpret_cast<InternalPage*>(t_page);
    page_id_t next_page_id = cur_page->lookup(key, comparator_);
    if(!(page = buffer_pool_manager_->FetchPage(next_page_id))){
      return false;
    }
    t_page = reinterpret_cast<BPlusTreePage*>(page->GetData());
    buffer_pool_manager_->UnpinPage(next_page_id, false);
  }
  /*->leaf*/
  if(!t_page->IsLeafPage()){
    return false;
  }
  LOG_DEBUG("# find a leaf=%d", t_page->GetPageId());
  auto cur_page = reinterpret_cast<LeafPage*>(t_page);
  ValueType v;
  if(cur_page->k_to_v(key, v, comparator_)){
    result->push_back(v);
    return true;
  }
  return false;
}


/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * Insert constant key & value pair into b+ tree
 * if current tree is empty, start new tree, update root page id and insert
 * entry, otherwise insert into leaf page.
 * @return: since we only support unique key, if user try to insert duplicate
 * keys return false, otherwise return true.
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value, Transaction *transaction) -> bool {
  Page *page;
  page_id_t pid;
  if(IsEmpty()){
    page = buffer_pool_manager_->NewPage(&pid);
    assert(page != nullptr);
    auto cur_page = reinterpret_cast<LeafPage*>(page->GetData());
    cur_page->Init(pid, INVALID_PAGE_ID, leaf_max_size_);
    
    //update_root
    Update_root(pid);
  }else{
    page = buffer_pool_manager_->FetchPage(root_page_id_);
    assert(page != nullptr);  
    LOG_DEBUG("# root=%d", root_page_id_);
  }
  /*internal->...->internal->*/
  BPlusTreePage *t_page = reinterpret_cast<BPlusTreePage*>(page->GetData());
  buffer_pool_manager_->UnpinPage(root_page_id_, false);
  while(t_page->IsRootPage()){
    LOG_DEBUG("# find a internal=%d", t_page->GetPageId());
    auto cur_page = reinterpret_cast<InternalPage*>(t_page);
    page_id_t next_page_id = cur_page->lookup(key, comparator_);
    if(!(page = buffer_pool_manager_->FetchPage(next_page_id))){
      return false;
    }
    t_page = reinterpret_cast<BPlusTreePage*>(page->GetData());
    buffer_pool_manager_->UnpinPage(next_page_id, false);
  }
  /*->leaf*/
  LOG_DEBUG("# find a leaf=%d", t_page->GetPageId());
  auto cur_page = reinterpret_cast<LeafPage*>(t_page);
  /*insert into leaf*/
  return cur_page->insert(key, value, comparator_, this);
}

/*my function*/
INDEX_TEMPLATE_ARGUMENTS
BPlusTreePage* BPLUSTREE_TYPE::pid_to_page(page_id_t pid){
  Page *page = buffer_pool_manager_->FetchPage(pid);
  if(!page) return nullptr;
  auto rs = reinterpret_cast<BPlusTreePage*>(page->GetData());
  buffer_pool_manager_->UnpinPage(pid, false);
  return rs;
}
/*my function*/
INDEX_TEMPLATE_ARGUMENTS
typename BPLUSTREE_TYPE::InternalPage* BPLUSTREE_TYPE::new_internal_page(page_id_t &nid, page_id_t parent){
  Page *page = buffer_pool_manager_->NewPage(&nid);
  assert(page != nullptr);
  auto cur_page = reinterpret_cast<BPLUSTREE_TYPE::InternalPage*>(page->GetData());
  cur_page->Init(nid, parent, internal_max_size_);
  buffer_pool_manager_->UnpinPage(nid, false);
  return cur_page;
}
/*my function*/
//new_leaf id=SelfId, set its NextPageId, set its ParentId
INDEX_TEMPLATE_ARGUMENTS
typename BPLUSTREE_TYPE::LeafPage* BPLUSTREE_TYPE::new_leaf_page(page_id_t &SelfId, page_id_t NextPageId, page_id_t ParentId){
  Page *page = buffer_pool_manager_->NewPage(&SelfId);
  assert(page != nullptr);
  auto cur_page = reinterpret_cast<BPLUSTREE_TYPE::LeafPage*>(page->GetData());
  cur_page->Init(SelfId, ParentId, leaf_max_size_);
  cur_page->SetNextPageId(NextPageId);
  buffer_pool_manager_->UnpinPage(SelfId, false);
  
  return cur_page;
}
/**/





/*****************************************************************************
 * REMOVE
 *****************************************************************************/
/*
 * Delete key & value pair associated with input key
 * If current tree is empty, return immdiately.
 * If not, User needs to first find the right leaf page as deletion target, then
 * delete entry from leaf page. Remember to deal with redistribute or merge if
 * necessary.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key, Transaction *transaction) {
  if(IsEmpty()) return;
  Page *page = buffer_pool_manager_->FetchPage(root_page_id_);
  /*internal->...->internal->*/
  BPlusTreePage *t_page = reinterpret_cast<BPlusTreePage*>(page->GetData());
  buffer_pool_manager_->UnpinPage(root_page_id_, false);
  while(t_page->IsRootPage()){
    auto cur_page = reinterpret_cast<InternalPage*>(t_page);
    page_id_t next_page_id = cur_page->lookup(key, comparator_);
    if(!(page = buffer_pool_manager_->FetchPage(next_page_id))){
      return;
    }
    t_page = reinterpret_cast<BPlusTreePage*>(page->GetData());
    buffer_pool_manager_->UnpinPage(next_page_id, false);
  }
  /*->leaf*/
  auto cur_page = reinterpret_cast<LeafPage*>(t_page);
  /*remove from leaf*/
  cur_page->remove(key, comparator_, this);
}

/*****************************************************************************
 * INDEX ITERATOR
 *****************************************************************************/
/*
 * Input parameter is void, find the leaftmost leaf page first, then construct
 * index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/*
 * Input parameter is low key, find the leaf page that contains the input key
 * first, then construct index iterator
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/*
 * Input parameter is void, construct an index iterator representing the end
 * of the key/value pair in the leaf node
 * @return : index iterator
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/**
 * @return Page id of the root of this tree
 */
INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { return root_page_id_; }

/*****************************************************************************
 * UTILITIES AND DEBUG
 *****************************************************************************/
/*
 * Update/Insert root page id in header page(where page_id = 0, header_page is
 * defined under include/page/header_page.h)
 * Call this method everytime root page id is changed.
 * @parameter: insert_record      defualt value is false. When set to true,
 * insert a record <index_name, root_page_id> into header page instead of
 * updating it.
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::UpdateRootPageId(int insert_record) {
  auto *header_page = static_cast<HeaderPage *>(buffer_pool_manager_->FetchPage(HEADER_PAGE_ID));
  if (insert_record != 0) {
    // create a new record<index_name + root_page_id> in header_page
    header_page->InsertRecord(index_name_, root_page_id_);
  } else {
    // update root_page_id in header_page
    header_page->UpdateRecord(index_name_, root_page_id_);
  }
  buffer_pool_manager_->UnpinPage(HEADER_PAGE_ID, true);
}

/*
 * This method is used for test only
 * Read data from file and insert one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::InsertFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;

    KeyType index_key;
    index_key.SetFromInteger(key);
    RID rid(key);
    Insert(index_key, rid, transaction);
  }
}
/*
 * This method is used for test only
 * Read data from file and remove one by one
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveFromFile(const std::string &file_name, Transaction *transaction) {
  int64_t key;
  std::ifstream input(file_name);
  while (input) {
    input >> key;
    KeyType index_key;
    index_key.SetFromInteger(key);
    Remove(index_key, transaction);
  }
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Draw(BufferPoolManager *bpm, const std::string &outf) {
  if (IsEmpty()) {
    LOG_WARN("Draw an empty tree");
    return;
  }
  std::ofstream out(outf);
  out << "digraph G {" << std::endl;
  ToGraph(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(root_page_id_)->GetData()), bpm, out);
  out << "}" << std::endl;
  out.close();
}

/**
 * This method is used for debug only, You don't need to modify
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Print(BufferPoolManager *bpm) {
  if (IsEmpty()) {
    LOG_WARN("Print an empty tree");
    return;
  }
  ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(root_page_id_)->GetData()), bpm);
}

/**
 * This method is used for debug only, You don't need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 * @param out
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToGraph(BPlusTreePage *page, BufferPoolManager *bpm, std::ofstream &out) const {
  std::string leaf_prefix("LEAF_");
  std::string internal_prefix("INT_");
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    // Print node name
    out << leaf_prefix << leaf->GetPageId();
    // Print node properties
    out << "[shape=plain color=green ";
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">P=" << leaf->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << leaf->GetSize() << "\">"
        << "max_size=" << leaf->GetMaxSize() << ",min_size=" << leaf->GetMinSize() << ",size=" << leaf->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < leaf->GetSize(); i++) {
      out << "<TD>" << leaf->KeyAt(i) << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Leaf node link if there is a next page
    if (leaf->GetNextPageId() != INVALID_PAGE_ID) {
      out << leaf_prefix << leaf->GetPageId() << " -> " << leaf_prefix << leaf->GetNextPageId() << ";\n";
      out << "{rank=same " << leaf_prefix << leaf->GetPageId() << " " << leaf_prefix << leaf->GetNextPageId() << "};\n";
    }

    // Print parent links if there is a parent
    if (leaf->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << leaf->GetParentPageId() << ":p" << leaf->GetPageId() << " -> " << leaf_prefix
          << leaf->GetPageId() << ";\n";
    }
  } else {
    auto *inner = reinterpret_cast<InternalPage *>(page);
    // Print node name
    out << internal_prefix << inner->GetPageId();
    // Print node properties
    out << "[shape=plain color=pink ";  // why not?
    // Print data of the node
    out << "label=<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    // Print data
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">P=" << inner->GetPageId() << "</TD></TR>\n";
    out << "<TR><TD COLSPAN=\"" << inner->GetSize() << "\">"
        << "max_size=" << inner->GetMaxSize() << ",min_size=" << inner->GetMinSize() << ",size=" << inner->GetSize()
        << "</TD></TR>\n";
    out << "<TR>";
    for (int i = 0; i < inner->GetSize(); i++) {
      out << "<TD PORT=\"p" << inner->ValueAt(i) << "\">";
      if (i > 0) {
        out << inner->KeyAt(i);
      } else {
        out << " ";
      }
      out << "</TD>\n";
    }
    out << "</TR>";
    // Print table end
    out << "</TABLE>>];\n";
    // Print Parent link
    if (inner->GetParentPageId() != INVALID_PAGE_ID) {
      out << internal_prefix << inner->GetParentPageId() << ":p" << inner->GetPageId() << " -> " << internal_prefix
          << inner->GetPageId() << ";\n";
    }
    // Print leaves
    for (int i = 0; i < inner->GetSize(); i++) {
      auto child_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i))->GetData());
      ToGraph(child_page, bpm, out);
      if (i > 0) {
        auto sibling_page = reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(inner->ValueAt(i - 1))->GetData());
        if (!sibling_page->IsLeafPage() && !child_page->IsLeafPage()) {
          out << "{rank=same " << internal_prefix << sibling_page->GetPageId() << " " << internal_prefix
              << child_page->GetPageId() << "};\n";
        }
        bpm->UnpinPage(sibling_page->GetPageId(), false);
      }
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

/**
 * This function is for debug only, you don't need to modify
 * @tparam KeyType
 * @tparam ValueType
 * @tparam KeyComparator
 * @param page
 * @param bpm
 */
INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::ToString(BPlusTreePage *page, BufferPoolManager *bpm) const {
  if (page->IsLeafPage()) {
    auto *leaf = reinterpret_cast<LeafPage *>(page);
    std::cout << "Leaf Page: " << leaf->GetPageId() << " parent: " << leaf->GetParentPageId()
              << " next: " << leaf->GetNextPageId() << std::endl;
    for (int i = 0; i < leaf->GetSize(); i++) {
      std::cout << leaf->KeyAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
  } else {
    auto *internal = reinterpret_cast<InternalPage *>(page);
    std::cout << "Internal Page: " << internal->GetPageId() << " parent: " << internal->GetParentPageId() << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      std::cout << internal->KeyAt(i) << ": " << internal->ValueAt(i) << ",";
    }
    std::cout << std::endl;
    std::cout << std::endl;
    for (int i = 0; i < internal->GetSize(); i++) {
      ToString(reinterpret_cast<BPlusTreePage *>(bpm->FetchPage(internal->ValueAt(i))->GetData()), bpm);
    }
  }
  bpm->UnpinPage(page->GetPageId(), false);
}

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
