#include <cstddef>
#include <cstdint>
#include <stack>
#include <string_view>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "file_meta.hpp"
#include "node.hpp"




template <std::size_t KeysCount>
class BLinkTree {
private:
  using KeyType = uint64_t;
  using PtrType = off_t;
  // TODO
public:
  BLinkTree();

  void Insert(KeyType key, std::string&& record);

  PtrType ReadMeta() const;

private:
  Node<KeysCount>* Get(PtrType ptr) const;

private:
  int fd;
  char* mapping;
  // PtrType root;

  constexpr static std::string_view path_ = "./test/database.bin";
};


template <std::size_t KeysCount>
BLinkTree<KeysCount>::BLinkTree() {
  fd = open(path_.data(), O_RDWR);
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
}

template <std::size_t KeysCount>
void BLinkTree<KeysCount>::Insert(KeyType key, std::string&& record) {
  std::stack<PtrType> stack;

  PtrType current_ptr = ReadMeta();
  
  // (mapping + current) - указатель на текущую вершину
  Node<KeysCount>* current = Get(current_ptr);

  while (!current->IsLeaf()) {
    auto tmp = current;
    current = current->ScanNodeFor();
  }
}

template <std::size_t KeysCount>
BLinkTree<KeysCount>::PtrType
  BLinkTree<KeysCount>::ReadMeta() const {
  FileMeta* meta = GetMetaLocked(fd, mapping);

  auto root = meta->root_offset;

  DispatchMeta(fd, mapping, meta);
}

/*
 * Private methods
*/

/*
 * Reads record from mapping with 'ptr' offset
*/
template <std::size_t KeysCount>
Node<KeysCount>* BLinkTree<KeysCount>::Get(PtrType ptr) const {
  return reinterpret_cast<Node<KeysCount>*>(mapping + ptr);
}



/*
*
* Все процессы делают маппинг файла дерева
*
* Для записи надо:
*   fcntl() для блокирования страниц
*   mlock() для невытеснения промежуточных данных
*
*   (при этом читатели должны уметь спокойно читать)
*   !!! Надо сделать мапинг для записи приватным !!!
*   То есть что-то типа remap_file_pages
*   Тогда проомежуточные изменения не будут видны читающим процессам
*
*   fcntl() ---
*   munlock() ---
*
*/
