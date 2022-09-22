//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.h
//
// Identification: src/include/buffer/lru_k_replacer.h
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <limits>
#include <list>
#include <mutex>  // NOLINT
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include<algorithm>

#include "common/config.h"
#include "common/macros.h"

namespace bustub {
struct node {
	int times;
	frame_id_t id;
	bool evictable;
	node* next, * pre;
	node(int id,int times=1) :times(times),evictable(true), id(id),next(nullptr),pre(nullptr) {}
	
};

class my_list {
private:
	node* front_, * back_, * queue, * k_queue, * evict;
	//front->q1(unevictable)->q2->q3(queue)->k1->k2(k_queue)->back
	int _size_;
	int cur;
	int k;
public:
	my_list(int size,int k){
		_size_ = size;
		k = k;
		cur = 0;
		front_ = new node(-1);
		back_ = new node(-1);
		front_->next = back_;
		back_->pre = front_;
		//
		queue  = front_;
		k_queue = back_;
	}
	~my_list(){}
	node* find(frame_id_t id) {
		node* p = front_->next;
		while (p != back_) {
			if (p->id == id) {
				return p;
			}
			p = p->next;
		}
		return nullptr;

	}
	void insert(node* first, node* second, node* tar) {
		cur++;
		first->next = tar;
		second->pre = tar;
		tar->pre = first;
		tar->next = second;
		//
		if (queue == first) {
			queue = tar;
		}
	}
	void del(node* tar) {
		tar->pre->next = tar->next;
		tar->next->pre = tar->pre;
		cur--;
		//
		if (queue == tar) {
			queue = tar->pre;
		}
	}
	void add(frame_id_t id) {
		node* it = find(id);
		if (!it) {
			//not found
			node* tmp = new node(id);
			if (cur >= _size_) {
				//need evict
				node* p = front_->next;
				while (!p->evictable) {
					p = p->next;
				}
				del(p);
			}
			insert(queue, queue->next, tmp);
		}
		else{
			if (++(it->times) < k) {
				del(it);
				insert(queue, queue->next, it);
			}
			else {
				del(it);
				insert(k_queue->pre, k_queue, it);
				//k_queue = it;
			}
		}
	}
	void evcit(frame_id_t &id) {
		node* p = front_->next;
		while (!p->evictable) {
			p = p->next;
		}
		id = p->id;
		del(p);
		delete p;
	}
	void remove(frame_id_t id) {
		node* it = find(id);
		if (it) {
			del(it);
			delete it;
		}
	}
	void set(frame_id_t id,bool evictable) {
		node* it = find(id);
		if (it) {
			if (it->evictable && !evictable) {
				cur--;
			}
			else if (!(it->evictable) && evictable) {
				cur++;
			}
			it->evictable = evictable;
		}
	}
	int get_cur() {
		return cur;
	}
};

/**
 * LRUKReplacer implements the LRU-k replacement policy.
 *
 * The LRU-k algorithm evicts a frame whose backward k-distance is maximum
 * of all frames. Backward k-distance is computed as the difference in time between
 * current timestamp and the timestamp of kth previous access.
 *
 * A frame with less than k historical references is given
 * +inf as its backward k-distance. When multipe frames have +inf backward k-distance,
 * classical LRU algorithm is used to choose victim.
 */
class LRUKReplacer {
 public:
  /**
   *
   * TODO(P1): Add implementation
   *
   * @brief a new LRUKReplacer.
   * @param num_frames the maximum number of frames the LRUReplacer will be required to store
   */
  explicit LRUKReplacer(size_t num_frames, size_t k);

  DISALLOW_COPY_AND_MOVE(LRUKReplacer);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Destroys the LRUReplacer.
   */
  ~LRUKReplacer() = default;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Find the frame with largest backward k-distance and evict that frame. Only frames
   * that are marked as 'evictable' are candidates for eviction.
   *
   * A frame with less than k historical references is given +inf as its backward k-distance.
   * If multiple frames have inf backward k-distance, then evict frame with earliest timestamp
   * based on LRU.
   *
   * Successful eviction of a frame should decrement the size of replacer and remove the frame's
   * access history.
   *
   * @param[out] frame_id id of frame that is evicted.
   * @return true if a frame is evicted successfully, false if no frames can be evicted.
   */
  auto Evict(frame_id_t *frame_id) -> bool;

  /**
   * TODO(P1): Add implementation
   *
   * @brief Record the event that the given frame id is accessed at current timestamp.
   * Create a new entry for access history if frame id has not been seen before.
   *
   * If frame id is invalid (ie. larger than replacer_size_), throw an exception. You can
   * also use BUSTUB_ASSERT to abort the process if frame id is invalid.
   *
   * @param frame_id id of frame that received a new access.
   */
  void RecordAccess(frame_id_t frame_id);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Toggle whether a frame is evictable or non-evictable. This function also
   * controls replacer's size. Note that size is equal to number of evictable entries.
   *
   * If a frame was previously evictable and is to be set to non-evictable, then size should
   * decrement. If a frame was previously non-evictable and is to be set to evictable,
   * then size should increment.
   *
   * If frame id is invalid, throw an exception or abort the process.
   *
   * For other scenarios, this function should terminate without modifying anything.
   *
   * @param frame_id id of frame whose 'evictable' status will be modified
   * @param set_evictable whether the given frame is evictable or not
   */
  void SetEvictable(frame_id_t frame_id, bool set_evictable);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Remove an evictable frame from replacer, along with its access history.
   * This function should also decrement replacer's size if removal is successful.
   *
   * Note that this is different from evicting a frame, which always remove the frame
   * with largest backward k-distance. This function removes specified frame id,
   * no matter what its backward k-distance is.
   *
   * If Remove is called on a non-evictable frame, throw an exception or abort the
   * process.
   *
   * If specified frame is not found, directly return from this function.
   *
   * @param frame_id id of frame to be removed
   */
  void Remove(frame_id_t frame_id);

  /**
   * TODO(P1): Add implementation
   *
   * @brief Return replacer's size, which tracks the number of evictable frames.
   *
   * @return size_t
   */
  auto Size() -> size_t;

 private:
  // TODO(student): implement me! You can replace these member variables as you like.
  // Remove maybe_unused if you start using them.
  [[maybe_unused]] size_t current_timestamp_{0};
  [[maybe_unused]] size_t curr_size_{0};
  [[maybe_unused]] size_t replacer_size_;
  [[maybe_unused]] size_t k_;
  my_list lst;
  std::mutex latch_;
};

}  // namespace bustub
