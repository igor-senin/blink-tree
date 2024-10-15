#include "node.hpp"

#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>


template <std::size_t KeysCount>
bool RLockNode(int fd, off_t offset);

template <std::size_t KeysCount>
Node<KeysCount>* RGetRawLocked(int fd, off_t offset, char* mapping);

template <std::size_t KeysCount>
void WLockNode(int fd, off_t offset);

template <std::size_t KeysCount>
void UnlockNode(int fd, off_t offset);


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
Node<KeysCount>* RGetRawLocked(int fd, off_t offset, char* mapping) {
  if (RLockNode<KeysCount>(fd, offset)) {
    // range is locked
    return reinterpret_cast<Node<KeysCount>*>(mapping + offset);
  }

  // unlucky
  size_t size = sizeof(Node<KeysCount>);
  void* node = malloc(size);
  if (pread(fd, node, size, offset) != size) {
    // TODO: better error checks
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

  // TODO: error check
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

  // TODO: error check
  (void)fcntl(fd, F_SETLKW, &flockstr);
}
