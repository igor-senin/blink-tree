#ifndef LOCKS_HPP
#define LOCKS_HPP

#include "node.hpp"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


template <std::size_t KeysCount>
bool RLockNode(int fd, off_t offset);

template <std::size_t KeysCount>
Node<KeysCount>* GetRaw(off_t offset, char* mapping);

template <std::size_t KeysCount>
Node<KeysCount>* RGetRawLocked(int fd, off_t offset, char* mapping);

template <std::size_t KeysCount>
void WLockNode(int fd, off_t offset);

template <std::size_t KeysCount>
void UnlockNode(int fd, off_t offset);

template <std::size_t KeysCount>
void PinNodeInRAM(Node<KeysCount>* node);

template <std::size_t KeysCount>
void UnpinNode(Node<KeysCount>* node);

template <std::size_t KeysCount>
void RDispatchNode(int fd, off_t offset, char* mapping);


/*
 * 
*/
template <std::size_t KeysCount>
bool RLockNode(int fd, off_t offset) {
  struct flock flockstr {
    .l_type = F_RDLCK,
    .l_whence = SEEK_SET,
    .l_start = offset,
    .l_len = sizeof(Node<KeysCount>)
  };

  return fcntl(fd, F_SETLK, &flockstr) != -1;
}

/*
 *
*/
template <std::size_t KeysCount>
Node<KeysCount>* GetRaw(off_t offset, char* mapping) {
  return reinterpret_cast<Node<KeysCount>*>(mapping + offset);
}

/*
*
*/
template <std::size_t KeysCount>
Node<KeysCount>* RGetRawLocked(int fd, off_t offset, char* mapping) {
  if (RLockNode<KeysCount>(fd, offset)) {
    // range is locked
    return GetRaw<KeysCount>(offset, mapping);
  }

  // unlucky
  size_t size = sizeof(Node<KeysCount>);
  void* node = malloc(size);
  if (pread(fd, node, size, offset) != size) {
    perror("pread");
    return nullptr;
  }
  return node;
}

/*
*
*/
template <std::size_t KeysCount>
void WLockNode(int fd, off_t offset) {
  struct flock flockstr {
    .l_type = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start = offset,
    .l_len = sizeof(Node<KeysCount>)
  };

  (void)fcntl(fd, F_SETLKW, &flockstr);
}

/*
*
*/
template <std::size_t KeysCount>
void UnlockNode(int fd, off_t offset) {
  struct flock flockstr {
    .l_type = F_UNLCK,
    .l_whence = SEEK_SET,
    .l_start = offset,
    .l_len = sizeof(Node<KeysCount>)
  };

  (void)fcntl(fd, F_SETLKW, &flockstr);
}

/*
 *
*/
template <std::size_t KeysCount>
void PinNodeInRAM(Node<KeysCount>* node) {
  if (mlock(node, sizeof(*node)) == -1) {
    perror("mlock");
    exit(EXIT_FAILURE);
  }
}

/*
 *
*/
template <std::size_t KeysCount>
void UnpinNode(Node<KeysCount>* node) {
  if (munlock(node, sizeof(*node)) == -1) {
    perror("munlock");
    exit(EXIT_FAILURE);
  }
}

/*
*
*/
template <std::size_t KeysCount>
void RDispatchNode(int fd, off_t offset, char* mapping, void* node) {
  if (node == offset + mapping) {
    UnlockNode<KeysCount>(fd, offset);
  } else {
    free(node);
  }
}

#endif  // LOCKS_HPP
