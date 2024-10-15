#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include <sys/types.h>


/*
 * TODO
*/
template <std::size_t KeysCount>
class Node {
private:
  using KeyType = std::uint64_t;
  using PtrType = off_t;

  std::array<KeyType, KeysCount + 1> keys_;
  std::array<PtrType, KeysCount + 1> ptrs_;

  std::size_t arr_size_{0};

  PtrType lintPtr_{0};

  uint64_t flags;
  // flags' bits:
  // 0: Node is leaf

public:
  Node() = default;
  Node& operator=(const Node&) = default;
  Node(const Node&) = default;

  ~Node();

  void Insert(KeyType key, std::string&& record);

  void SplitNode();

  bool IsLeaf() const;

} __attribute__((packed)); // TODO: packed needed?



template <std::size_t KeysCount>
void Node<KeysCount>::Insert(KeyType key, std::string&& record) {
  // TODO: должна быть блокировка на эту Ноду
  auto begin = keys_.begin();
  auto end = begin + arr_size_;
  auto pos = std::lower_bound(begin, end, key);

  auto ptrs_begin = ptrs_.begin();
  auto ptrs_end = ptrs_.begin() + arr_size_;
  auto ptrs_pos = ptrs_begin + (pos - begin);
  
  if (pos < end) {
    std::copy_backward(pos, end, end + 1);
    std::copy_backward(ptrs_pos, ptrs_end, ptrs_end + 1);
  }

  *pos = key;
  *ptrs_pos = 0; // TODO: create place for new record
  arr_size_++;

  if (arr_size_ == KeysCount + 1) {
    // TODO: node is unsafe; need to split
    SplitNode();
  }
}

template <std::size_t KeysCount>
void Node<KeysCount>::SplitNode() {
  auto split_pos =0 ;
  // TODO
}

template <std::size_t KeysCount>
bool Node<KeysCount>::IsLeaf() const {
  return flags & 1;
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
