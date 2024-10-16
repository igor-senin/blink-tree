#ifndef NODE_HPP
#define NODE_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <sys/types.h>


/*
 * TODO
*/
template <std::size_t KeysCount>
class Node {
private:
  using KeyType = std::uint64_t;
  using PtrType = off_t;

  // +1 for insertion in unsafe node
  std::array<KeyType, KeysCount + 1> keys_;
  std::array<PtrType, KeysCount + 1> ptrs_;

  std::size_t arr_size_{0};

  PtrType link_ptr_{0};

  std::uint32_t level_{0};
  // Leafs have level_ 0;
  // root has maximum level_.

  std::uint32_t flags_;
  // flags' bits:
  // 0: Node is leaf
  // 1: Node is root

public:
  Node() = default;
  Node& operator=(const Node&) = default;
  Node(const Node&) = default;

  ~Node();

  auto/*PtrType*/ ScanNodeFor(KeyType key) const;
  void Insert(KeyType key, PtrType record_ptr);

  void Rearrange(Node* new_node, PtrType ptr);

  bool Contains(KeyType key) const;

  auto/*KeyType*/ GetMaxKey() const;

  bool IsLeaf() const;
  bool IsInner() const;
  bool IsRoot() const;
  bool IsSafe() const;

  auto/*PtrType*/ LinkPointer() const;
  std::uint32_t Level() const;

} __attribute__((packed)); // TODO: packed needed?


template <std::size_t KeysCount>
auto Node<KeysCount>::ScanNodeFor(KeyType key) const {
  auto begin = keys_.begin();
  auto end = begin + arr_size_;
  auto pos = std::upper_bound(begin, end, key);

  return pos == end ? LinkPointer() : ptrs_[pos - begin];
}

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

template <std::size_t KeysCount>
void Node<KeysCount>::Rearrange(Node* new_node, PtrType ptr) {
  /* Our node is unsafe, i.e. contains 
   * (2*KeysCount + 1) keys and ptrs.
   * new_node is newly allocated node.
   * [0, ..., KeysCount] -> this;
   * [KeysCount+1, ..., 2*KeysCount] -> new_node. */
  std::copy(
    keys_.begin() + KeysCount + 1,
    keys_.end(),
    new_node->keys_.begin()
  );
  std::copy(
    ptrs_.begin() + KeysCount + 1,
    ptrs_.end(),
    new_node->ptrs_.begin()
  );

  new_node->arr_size_ = KeysCount;
  arr_size_ = KeysCount + 1;
  
  new_node->link_ptr_ = link_ptr_;
  link_ptr_ = ptr;

  new_node->level_ = level_;
  new_node->flags_ = flags_; /* TODO: currently so */
}

template <std::size_t KeysCount>
bool Node<KeysCount>::Contains(KeyType key) const {
  auto begin = keys_.begin();
  auto end = begin + arr_size_;
  auto pos = std::lower_bound(begin, end, key);

  return pos != end && *pos == key;
}

template <std::size_t KeysCount>
inline auto Node<KeysCount>::GetMaxKey() const {
  return keys_[arr_size_ - 1];
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
  return arr_size_ < KeysCount;
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
* 2k keys => Node is unsafe
*
*****************************************************/

#endif  // NODE_HPP
