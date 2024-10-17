#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stack>
#include <string_view>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "allocate_data.hpp"
#include "file_meta.h"
#include "locks.hpp"
#include "node.hpp"




template <std::size_t KeysCount>
class BLinkTree {
private:
  using KeyType = std::uint64_t;
  using PtrType = off_t;

public:
  BLinkTree();

  ~BLinkTree();

  void Insert(KeyType key, std::string_view record);

  FileMeta ReadMeta() const;

private:
  auto/*PtrType*/ MoveRight(KeyType key, PtrType current_ptr);

  auto/*PtrType*/ UpdateDescend(KeyType key,
                                std::stack<PtrType>& stack,
                                std::uint32_t level);

private:
  int fd;
  char* mapping;
  size_t mapping_size;

  constexpr static std::string_view path_ = "./test/database.bin";
};




template <std::size_t KeysCount>
BLinkTree<KeysCount>::BLinkTree() {
  fd = open(path_.data(), O_RDWR);
  if (fd < 0) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  struct stat statbuf;
  if (fstat(fd, &statbuf)) {
    perror("fstat");
    exit(EXIT_FAILURE);
  }
  mapping_size = statbuf.st_size;
  mapping =
    (char*)mmap(
      NULL,
      mapping_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fd,
      0
    );
  if (mapping == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }
  if (madvise(mapping, mapping_size, MADV_RANDOM) == -1) {
    perror("madvise");
    exit(EXIT_FAILURE);
  }
}



template <std::size_t KeysCount>
BLinkTree<KeysCount>::~BLinkTree() {
  munmap(mapping, mapping_size);

  close(fd);
}



/*
 * Insert -
 * insert (key, record) into BlinkTree.
 *
 * @key: key that should be inserted.
 * @record: string_view to record.
*/
template <std::size_t KeysCount>
void BLinkTree<KeysCount>::Insert(KeyType key, std::string_view record) {
  std::stack<PtrType> stack;

  PtrType current_ptr = UpdateDescend(key, stack, 0);
  Node<KeysCount>* current = GetRaw<KeysCount>(current_ptr, mapping);

  /* Now current[_ptr] is leaf;
   * no locks being hold. */

  current_ptr = MoveRight(key, current_ptr);

  /* Note: current is under WriteLock now */

  if (current->Contains(key)) {
    UnlockNode<KeysCount>(fd, current_ptr);
    return;
  }

  PtrType record_ptr = AllocateRecord(fd, record);

  // DoInsertion:
  while (true) {
    current->Insert(key, record_ptr);
    if (current->IsSafe()) {
      UnlockNode<KeysCount>(fd, current_ptr);
      break;
    }
    if (current->IsRoot()) {
      /* 'current' is root and unsafe.
       * Create 2 new nodes (new root and right son of new root). */
      PtrType new_root_ptr = AllocateNode<KeysCount>(fd);
      PtrType right_son_ptr = AllocateNode<KeysCount>(fd);
      auto new_root = GetRaw<KeysCount>(new_root_ptr, mapping);
      auto right_son = GetRaw<KeysCount>(right_son_ptr, mapping);
      current->RearrangeRoot(new_root, new_root_ptr,
                             right_son, right_son_ptr,
                             current_ptr);
      // TODO:
      // Flush:
      // MSync(right_son_ptr);
      // MSync(new_root_ptr);
      // MSync(current_ptr);
      // TODO: UpdateMeta;
      UnlockNode<KeysCount>(fd, current_ptr);
      break;
    }

    PtrType new_node_ptr = AllocateNode<KeysCount>(fd);
    Node<KeysCount>* new_node = GetRaw<KeysCount>(new_node_ptr, mapping);
    current->Rearrange(new_node, new_node_ptr);

    key = new_node->GetMinKey();
    record_ptr = new_node_ptr;

    // TODO
    // MSync(new_node);
    // MSync(current);

    auto level = current->Level();

    UnlockNode<KeysCount>(fd, current_ptr);

    if (stack.empty()) {
      /* Stack is empty, but 'current' isn't root.
       * Go again from root to current level,
       * saving right-most nodes in 'stack'. */
      UpdateDescend(key, stack, level);
    }

    current_ptr = stack.top();
    stack.pop();

    current_ptr = MoveRight(key, current_ptr);

    /* Note: current is under WriteLock now */

    current = GetRaw<KeysCount>(current_ptr, mapping);
  }
}



/*
 * ReadMeta - reads file meta.
 *
 * Returns:
 * @meta: copy of consistent file meta at some point.
*/
template <std::size_t KeysCount>
FileMeta BLinkTree<KeysCount>::ReadMeta() const {
  FileMeta* meta = GetMetaLocked(fd, mapping);

  FileMeta copy{*meta};

  DispatchMeta(fd, mapping, meta);

  return copy;
}


/*
 * Private methods
*/


/*
 * MoveRight -
 * go right on current tree level
 * until we find proper node to insert key.
 *
 * @key: key that should be inserted.
 * @current_ptr: starting point
 * 
 * Returns:
 * @current_ptr: offset-pointer to WriteLocked node
 *   in which key should be inserted.
*/
template <std::size_t KeysCount>
auto BLinkTree<KeysCount>::MoveRight(KeyType key, PtrType current_ptr) {
  WLockNode<KeysCount>(fd, current_ptr);

  Node<KeysCount>* current = GetRaw<KeysCount>(current_ptr, mapping);

  while (true) {
    auto tmp_ptr = current->ScanNodeFor(key);
    if (tmp_ptr != current->LinkPointer()) {
      break;
    }
    WLockNode<KeysCount>(fd, tmp_ptr);
    UnlockNode<KeysCount>(fd, current_ptr);

    current = GetRaw<KeysCount>(tmp_ptr, mapping);
    current_ptr = tmp_ptr;
  }

  return current_ptr;
}



/*
 * UpdateDescend -
 * go down to level 'level' inclusively,
 * searching for place where 'key' could be inserted.
 * Right-most nodes are added to 'stack'.
 * 'stack' MUST be empty.
 *
 * @key: key that will be inserted
 * @stack: std::stack for saving right-most nodes
 * @level: go down until this level
 *
 * Returns:
 * @current_ptr: offset-pointer to RLocked node
 *   on needed level.
*/
template <std::size_t KeysCount>
auto BLinkTree<KeysCount>::UpdateDescend(
  KeyType key,
  std::stack<PtrType>& stack,
  std::uint32_t level) {

  assert(stack.empty() && "UpdateDescend: stack is not empty!");

  FileMeta meta = ReadMeta();
  PtrType current_ptr = meta.root_offset;
  Node<KeysCount>* current = RGetRawLocked<KeysCount>(fd, current_ptr, mapping);

  std::uint32_t current_level = current->Level();

  while (current_level != level && !current->IsLeaf()) {
    auto tmp = current;
    auto tmp_ptr = current_ptr;
    current_ptr = current->ScanNodeFor(key);
    if (current_ptr != tmp->LinkPointer()) {
      stack.push(tmp_ptr);
    }
    current = RGetRawLocked<KeysCount>(fd, current_ptr, mapping);
    RDispatchNode<KeysCount>(fd, tmp_ptr, mapping, tmp);

    current_level = current->Level();
  }

  RDispatchNode<KeysCount>(fd, current_ptr, mapping);
  return current_ptr;
}

