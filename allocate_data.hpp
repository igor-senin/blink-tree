#ifndef ALLOCATE_DATA_HPP
#define ALLOCATE_DATA_HPP

#include <cstddef>
#include <string_view>

#include <sys/types.h>

#include "allocate.h"
#include "node.hpp"


template <std::size_t KeysCount>
off_t AllocateNode(int fd);

off_t AllocateRecord(int fd, std::string_view record);



template <std::size_t KeysCount>
off_t AllocateNode(int fd) {
  // std::cout << "Allocate node: " << sizeof(Node<KeysCount>) << "\n";

  char buf[sizeof(Node<KeysCount>)] = {0};

  return Allocate(fd, buf, sizeof(Node<KeysCount>));
}

#endif  // ALLOCATE_DATA_HPP
