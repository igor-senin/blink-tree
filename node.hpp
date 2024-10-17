#ifndef NODE_HPP
#define NODE_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <sys/types.h>




template <std::size_t KeysCount>
class __attribute__((packed)) Node {
private:
  using KeyType = std::uint64_t;
  using PtrType = off_t;

  // +1 for insertion in unsafe node
  std::array<KeyType, KeysCount + 1> keys_;
  std::array<PtrType, KeysCount + 1> ptrs_;

  std::size_t arr_size_{0};

  PtrType link_ptr_{0};
  // Pointer to right sibling.

  std::uint32_t level_{0};
  // Leafs have level_ 0;
  // root has maximum level_.

  std::uint32_t flags_;
  // flags' bits:
  // 0: Node is leaf
  // 1: Node is root

  constexpr static std::size_t kInfValue = std::numeric_limits<std::size_t>::max();

public:
  Node() = default;
  Node& operator=(const Node&) = default;
  Node(const Node&) = default;

  ~Node();

  auto/*PtrType*/ ScanNodeFor(KeyType key) const;
  void Insert(KeyType key, PtrType record_ptr);

  void Rearrange(Node* new_node, PtrType new_node_ptr);
  void RearrangeRoot(Node* new_root, PtrType new_root_ptr,
                     Node* right_son, PtrType right_son_ptr,
                     PtrType current_ptr);

  bool Contains(KeyType key) const;

  auto/*KeyType*/ GetMaxKey() const;
  auto/*KeyType*/ GetMinKey() const;

  bool IsLeaf() const;
  bool IsInner() const;
  bool IsRoot() const;
  bool IsSafe() const;

  auto/*PtrType*/ LinkPointer() const;
  std::uint32_t Level() const;

};


/*
 *
*/
template <std::size_t KeysCount>
auto Node<KeysCount>::ScanNodeFor(KeyType key) const {
  auto begin = keys_.begin();
  auto end = begin + arr_size_;
  auto pos = std::upper_bound(begin, end, key);

  return pos == end ? LinkPointer() : ptrs_[pos - begin];
}



/*
 *
*/
template <std::size_t KeysCount>
void Node<KeysCount>::Insert(KeyType key, PtrType record_ptr) {
  /* Node MUST be under Write Lock */
  auto begin = keys_.begin();
  auto end = begin + arr_size_;
  auto pos = std::upper_bound(begin, end, key);

  auto ptrs_begin = ptrs_.begin();
  auto ptrs_end = ptrs_.begin() + arr_size_;
  auto ptrs_pos = ptrs_begin + (pos - begin);
  
  if (pos < end) {
    /* Place for new element */
    std::copy_backward(pos, end, end + 1);
    std::copy_backward(ptrs_pos, ptrs_end, ptrs_end + 1);
  }

  *pos = key;
  *ptrs_pos = record_ptr;
  arr_size_++;
}


/*
 *
*/
template <std::size_t KeysCount>
void Node<KeysCount>::Rearrange(Node* new_node, PtrType new_node_ptr) {
  /* Our node is unsafe, i.e. now (after insertion)
   * contains (2*KeysCount + 1) keys and ptrs.
   * new_node is newly allocated node.
   * [0, ..., KeysCount-1] -> this;
   * [KeysCount, ..., 2*KeysCount] -> new_node. */
  std::copy(
    keys_.begin() + KeysCount,
    keys_.end(),
    new_node->keys_.begin()
  );
  std::copy(
    ptrs_.begin() + KeysCount,
    ptrs_.end(),
    new_node->ptrs_.begin()
  );

  new_node->arr_size_ = KeysCount + 1;
  arr_size_ = KeysCount;
  
  new_node->link_ptr_ = link_ptr_;
  link_ptr_ = new_node_ptr;

  new_node->level_ = level_;
  new_node->flags_ = flags_;
}


/*
 *
*/
template <std::size_t KeysCount>
void Node<KeysCount>::RearrangeRoot(
  Node* new_root, PtrType new_root_ptr,
  Node* right_son, PtrType right_son_ptr,
  PtrType current_ptr) {
  /* Current Node MUST be root.
   * Create new root with two children: current_ptr and right_son_ptr. */

  assert(IsRoot() && "RearrangeRoot: called for non root");

  flags_ ^= 0b10; // no longer the root
  
  Rearrange(right_son, right_son_ptr); // split old root

  new_root->arr_size_ = 2;
  new_root->link_ptr_ = nullptr;
  new_root->level_ = level_ + 1;
  new_root->flags_ = 0b10; // indicating non-leaf root

  new_root->keys_ = {right_son->GetMinKey(), kInfValue};
  new_root->ptrs_ = {current_ptr, right_son_ptr};
}


/*
 *
*/
template <std::size_t KeysCount>
bool Node<KeysCount>::Contains(KeyType key) const {
  auto begin = keys_.begin();
  auto end = begin + arr_size_;
  auto pos = std::lower_bound(begin, end, key);

  return pos != end && *pos == key;
}

template <std::size_t KeysCount>
inline auto Node<KeysCount>::GetMaxKey() const {
  assert(arr_size_ > 0 && "GetMaxKey: Node is empty!");
  return keys_[arr_size_ - 1];
}

template <std::size_t KeysCount>
inline auto Node<KeysCount>::GetMinKey() const {
  assert(arr_size_ > 0 && "GetMinKey: Node is empty!");
  return keys_[0];
}

template <std::size_t KeysCount>
inline bool Node<KeysCount>::IsLeaf() const {
  return flags_ & 0b1;
}

template <std::size_t KeysCount>
inline bool Node<KeysCount>::IsRoot() const {
  return flags_ & 0b10;
}

template <std::size_t KeysCount>
inline bool Node<KeysCount>::IsInner() const {
  return !(IsLeaf() || IsRoot());
}

template <std::size_t KeysCount>
inline bool Node<KeysCount>::IsSafe() const {
  return arr_size_ <= 2 * KeysCount;
}

template <std::size_t KeysCount>
inline auto Node<KeysCount>::LinkPointer() const {
  return link_ptr_;
}

template <std::size_t KeysCount>
inline std::uint32_t Node<KeysCount>::Level() const {
  return level_;
}


// Node
// Key: uint64_t
// Val: off_t
/*****************************************************
*
* ||p_0 k_0||---||p_1 k_1||-- ..... --||p_{2k} k_{2k}||
*
* p_{2k+1} = link pointer to next right node
*
* 2k+1 keys => Node is unsafe (must be splitted)
*
*****************************************************/

#endif  // NODE_HPP
