#include <cstddef>
#include <cstdint>
#include <stack>
#include <string_view>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "file_meta.hpp"
#include "locks.hpp"
#include "node.hpp"




template <std::size_t KeysCount>
class BLinkTree {
private:
  using KeyType = uint64_t;
  using PtrType = off_t;
  // TODO

public:
  BLinkTree();

  void Insert(KeyType key, std::string_view record);

  FileMeta ReadMeta() const;

private:
  void DoInsertion(KeyType key, PtrType current_ptr, PtrType record_ptr);

  void UpdateDescend(KeyType key, std::stack<PtrType>& stack, std::uint32_t level);

  Node<KeysCount>* Get(PtrType ptr) const;

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
    // TODO: Обработать ошибку
    exit(1);
  }
  struct stat statbuf;
  if (fstat(fd, &statbuf)) {
    // TODO: обработать ошибку
    exit(1);
  }
  mapping =
    (char*)mmap(
      NULL,
      statbuf.st_size,
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fd,
      0
    );
  if (mapping == MAP_FAILED) {
    // TODO: Обработать ошибку
    exit(1);
  }
  mapping_size = statbuf.st_size;
  madvise(mapping, mapping_size, MADV_RANDOM); // TODO
}



template <std::size_t KeysCount>
void BLinkTree<KeysCount>::Insert(KeyType key, std::string_view record) {
  std::stack<PtrType> stack;

  FileMeta meta = ReadMeta();
  PtrType current_ptr = meta.root_offset;
  
  Node<KeysCount>* current = RGetRawLocked<KeysCount>(fd, current_ptr, mapping);

  while (!current->IsLeaf()) {
    auto tmp = current;
    auto tmp_ptr = current_ptr;
    current_ptr = current->ScanNodeFor(key);
    if (current_ptr != tmp->LinkPointer()) {
      stack.push(tmp_ptr);
    }
    current = RGetRawLocked<KeysCount>(fd, current_ptr, mapping);
    RDispatchNode<KeysCount>(fd, tmp_ptr, mapping, tmp);
  }

  /* Now current[_ptr] is needed leaf */
  RDispatchNode<KeysCount>(fd, current_ptr, mapping, current);
  WLockNode<KeysCount>(fd, current_ptr);

  // MoveRight:
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

  if (current->Contains(key)) {
    return;
  }

  // DoInsertion(key, record, current_ptr, stack)
  // DoInsertion:
  while (true) {
    if (current->IsSafe()) {
      current->Insert(key, record);
      UnlockNode<KeysCount>(fd, current_ptr);
      break;
    }
    if (current->IsRoot()) {
      // TODO
      break;
    }
    // PtrType new_node = AllocateNode();
    PtrType new_node_ptr = 0; // TODO
    auto new_node = GetRaw<KeysCount>(new_node_ptr, mapping);
    current->Rearrange(new_node);

    KeyType separator = current->GetMaxKey();

    // TODO
    // MSync(new_node);
    // MSync(current);

    if (stack.empty()) {
      // TODO: стэк пустой, но вершина не корень
      // Надо Снова пройти от корня до уровня current->Level()
      // и находить 
      // UpdateDescend(separator, stack, current->Level())
    }
  }
}



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


template <std::size_t KeysCount>
void BLinkTree<KeysCount>::DoInsertion(
  KeyType key,
  PtrType current_ptr,
  PtrType record_ptr) {
  /* TODO: smth MUST be locked */
  Node<KeysCount>* current = Get(current_ptr);
  if (current->IsSafe()) {
    current->Insert(key, record_ptr);
    MSync();
  }
}

/*
 * Спуститься с корня до уровня 'level' включительно
*/
template <std::size_t KeysCount>
void BLinkTree<KeysCount>::UpdateDescend(
  KeyType key,
  std::stack<PtrType>& stack,
  std::uint32_t level) {
  FileMeta meta = ReadMeta();
  PtrType current_ptr = meta.root_offset;
  Node<KeysCount>* current = RGetRawLocked<KeysCount>(fd, current_ptr, mapping);

  std::uint32_t current_level;

  do {
    current_level = current->Level();

    auto tmp = current;
    auto tmp_ptr = current_ptr;
    current_ptr = current->ScanNodeFor(key);
    if (current_ptr != tmp->LinkPointer()) {
      stack.push(tmp_ptr);
    }
    current = RGetRawLocked<KeysCount>(fd, current_ptr, mapping);
    RDispatchNode<KeysCount>(fd, tmp_ptr, mapping, tmp);

  } while (current_level != level);
}

/*
 * Reads record from mapping with 'ptr' offset
*/
template <std::size_t KeysCount>
Node<KeysCount>* BLinkTree<KeysCount>::Get(PtrType ptr) const {
  return reinterpret_cast<Node<KeysCount>*>(mapping + ptr);
}

