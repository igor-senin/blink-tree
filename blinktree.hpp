#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stack>
#include <string_view>

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "allocate_data.hpp"
#include "file_meta.h"
#include "locks.hpp"
#include "memory_sync.h"
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

  auto/*PtrType*/ GetRoot() const;

private:
  auto/*PtrType*/ MoveRight(KeyType key, PtrType current_ptr);

  auto/*PtrType*/ UpdateDescend(KeyType key,
                                std::stack<PtrType>& stack,
                                std::uint32_t level);

  Node<KeysCount>* GetRaw(PtrType ptr);

  Node<KeysCount>* RGetRawLocked(PtrType ptr);
  void RDispatchNode(PtrType ptr);

  void WLockNode(PtrType ptr);
  void UnlockNode(PtrType ptr);

  void PinNodeInRAM(PtrType ptr);
  void UnpinNode(PtrType ptr);

  void UpdateMappingIfNecessary(PtrType ptr);
  void UpdateMapping();

private:
  int fd;
  char* mapping;
  size_t mapping_size;

  constexpr static std::string_view path_ = "./test/database.bin";

  static constexpr auto NodeSize = sizeof(Node<KeysCount>);
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

  /* Now current_ptr is leaf;
   * no locks being hold. */

  current_ptr = MoveRight(key, current_ptr);

  /* Note: current is under WriteLock now */

  if (GetRaw(current_ptr)->Contains(key)) {
    UnlockNode(current_ptr);
    return;
  }

  PtrType record_ptr = AllocateRecord(fd, record);

  // DoInsertion:
  while (true) {
    PinNodeInRAM(current_ptr);
    GetRaw(current_ptr)->Insert(key, record_ptr);
    if (GetRaw(current_ptr)->IsSafe()) [[likely]] {
      UnpinNode(current_ptr);
      UnlockNode(current_ptr);
      break;
    }
    if (GetRaw(current_ptr)->IsRoot()) {
      /* 'current' is root and unsafe.
       * Create 2 new nodes (new root and right son of new root). */
      PtrType new_root_ptr = AllocateNode<KeysCount>(fd);
      PtrType right_son_ptr = AllocateNode<KeysCount>(fd);
      GetRaw(current_ptr)->RearrangeRoot(
        GetRaw(new_root_ptr), new_root_ptr,
        GetRaw(right_son_ptr), right_son_ptr,
        current_ptr
      );

      MSync(GetRaw(right_son_ptr), NodeSize);
      MSync(GetRaw(new_root_ptr), NodeSize);
      UnpinNode(current_ptr);
      MSync(GetRaw(current_ptr), NodeSize);

      UpdateMeta(fd, mapping, new_root_ptr, GetRaw(new_root_ptr)->Level());

      UnlockNode(current_ptr);

      break;
    }

    PtrType new_node_ptr = AllocateNode<KeysCount>(fd);
    GetRaw(current_ptr)->Rearrange(GetRaw(new_node_ptr), new_node_ptr);

    key = GetRaw(new_node_ptr)->GetMinKey();
    record_ptr = new_node_ptr;

    MSync(GetRaw(new_node_ptr), NodeSize);
    UnpinNode(current_ptr);
    MSync(GetRaw(current_ptr), NodeSize);

    auto level = GetRaw(current_ptr)->Level();

    UnlockNode(current_ptr);

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
 * GetRoot - get current tree root.
 *
 * Returns:
 * @root: offset-pointer to root.
*/
template <std::size_t KeysCount>
auto BLinkTree<KeysCount>::GetRoot() const {
  FileMeta* meta = GetMetaLocked(fd, mapping);

  PtrType root = meta->root_offset;

  DispatchMeta(fd, mapping, meta);

  return root;
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
  WLockNode(current_ptr);

  while (true) {
    auto tmp_ptr = GetRaw(current_ptr)->ScanNodeFor(key);
    if (tmp_ptr != GetRaw(current_ptr)->LinkPointer()) {
      break;
    }
    WLockNode(tmp_ptr);
    UnlockNode(current_ptr);

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

  PtrType current_ptr = GetRoot();
  Node<KeysCount>* current = RGetRawLocked(current_ptr);

  std::uint32_t current_level = current->Level();

  while (current_level != level && !current->IsLeaf()) {
    auto tmp_ptr = current_ptr;
    current_ptr = current->ScanNodeFor(key);
    if (current_ptr != GetRaw(tmp_ptr)->LinkPointer()) {
      stack.push(tmp_ptr);
    }
    current = RGetRawLocked(current_ptr);
    RDispatchNode(tmp_ptr);

    current_level = current->Level();
  }

  RDispatchNode(current_ptr);

  return current_ptr;
}



template <std::size_t KeysCount>
inline Node<KeysCount>* BLinkTree<KeysCount>::GetRaw(PtrType ptr) {
  UpdateMappingIfNecessary(ptr);
  return GetRaw<KeysCount>(ptr, mapping);
}


template <std::size_t KeysCount>
inline Node<KeysCount>* BLinkTree<KeysCount>::RGetRawLocked(PtrType ptr) {
  UpdateMappingIfNecessary(ptr);
  return RGetRawLocked<KeysCount>(fd, ptr, mapping);
}


template <std::size_t KeysCount>
inline void BLinkTree<KeysCount>::RDispatchNode(PtrType ptr) {
  return RDispatchNode<KeysCount>(fd, ptr, mapping, GetRaw(ptr));
}


template <std::size_t KeysCount>
inline void BLinkTree<KeysCount>::WLockNode(PtrType ptr) {
  UpdateMappingIfNecessary(ptr);
  WLockNode<KeysCount>(fd, ptr);
}


template <std::size_t KeysCount>
inline void BLinkTree<KeysCount>::UnlockNode(PtrType ptr) {
  UnlockNode<KeysCount>(fd, ptr);
}


template <std::size_t KeysCount>
inline void BLinkTree<KeysCount>::PinNodeInRAM(PtrType ptr) {
  PinNodeInRAM<KeysCount>(GetRaw(ptr));
}

template <std::size_t KeysCount>
inline void BLinkTree<KeysCount>::UnpinNode(PtrType ptr) {
  UnpinNode<KeysCount>(GetRaw(ptr));
}


template <std::size_t KeysCount>
void BLinkTree<KeysCount>::UpdateMappingIfNecessary(PtrType ptr) {
  if (ptr + NodeSize > mapping_size)
    [[unlikely]] {
    UpdateMapping();
  }
}


template <std::size_t KeysCount>
void BLinkTree<KeysCount>::UpdateMapping() {
  struct stat statbuf;
  if (fstat(fd, &statbuf))
    [[unlikely]] {
    perror("fstat");
    exit(EXIT_FAILURE);
  }
  mapping =
    (char*)mremap(
      mapping,
      mapping_size,
      statbuf.st_size,
      MREMAP_MAYMOVE
    );
  mapping_size = statbuf.st_size;
  if (mapping == MAP_FAILED)
    [[unlikely]] {
    perror("mremap");
    exit(EXIT_FAILURE);
  }
  if (madvise(mapping, mapping_size, MADV_RANDOM) == -1)
    [[unlikely]] {
    perror("madvise");
    exit(EXIT_FAILURE);
  }
}

