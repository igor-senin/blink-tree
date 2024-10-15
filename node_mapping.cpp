#include "node.hpp"

#include <sys/types.h>
#include <sys/mman.h>

template <std::size_t KeysCount>
Node<KeysCount>* LoadRaw(int fd, off_t offset) {
  void* ptr =
    mmap(
      NULL,
      sizeof(Node<KeysCount>),
      PROT_READ | PROT_WRITE,
      MAP_SHARED,
      fd, offset
    );

  return reinterpret_cast<Node<KeysCount>*>(ptr);
}

template <std::size_t KeysCount>
void PinNode(Node<KeysCount>* node) {
  mlock((void*)node, sizeof(*node));
}


template <std::size_t KeysCount>
void UnpinNode(Node<KeysCount>* node) {
  munlock((void*)node, sizeof(*node));
}
