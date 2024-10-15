#include "file_meta.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

bool RLockMeta(int fd) {
  struct flock flockstr {
    .l_type = F_RDLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = sizeof(FileMeta)
  };

  return fcntl(fd, F_SETLK, &flockstr) != -1;
}

FileMeta* GetMetaLocked(int fd, char* mapping) {
  if (RLockMeta(fd)) {
    return reinterpret_cast<FileMeta*>(mapping);
  }

  size_t size = sizeof(FileMeta);
  void* meta = malloc(size);
  if (pread(fd, meta, size, 0) != size) {
    // TODO: better error checks
    perror("pread");
    return nullptr;
  }

  return (FileMeta*)meta;
}

void WLockMeta(int fd) {
  struct flock flockstr {
    .l_type = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = sizeof(FileMeta)
  };

  // TODO
  (void)fcntl(fd, F_SETLKW, &flockstr);
}

void UnlockMeta(int fd) {
  struct flock flockstr {
    .l_type = F_UNLCK,
    .l_whence = 0,
    .l_start = 0,
    .l_len = sizeof(FileMeta)
  };

  // TODO
  (void)fcntl(fd, F_SETLKW, &flockstr);
}

void DispatchMeta(int fd, char* mapping, void* meta) {
  if ((void*)meta == mapping) {
    UnlockMeta(fd);
  } else {
    free(meta);
  }
}
