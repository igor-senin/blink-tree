#include "file_meta.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

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
    return (FileMeta*)mapping;
  }

  size_t size = sizeof(FileMeta);
  void* meta = malloc(size);
  if (meta == NULL) {
    perror("malloc");
    return nullptr;
  }
  if (pread(fd, meta, size, 0) != size) {
    perror("pread");
    free(meta);
    return nullptr;
  }

  return (FileMeta*)meta;
}

void WLockMeta(int fd, char* mapping) {
  struct flock flockstr {
    .l_type = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = sizeof(FileMeta)
  };

  (void)fcntl(fd, F_SETLKW, &flockstr);
}

void UnlockMeta(int fd, char* mapping) {
  struct flock flockstr {
    .l_type = F_UNLCK,
    .l_whence = 0,
    .l_start = 0,
    .l_len = sizeof(FileMeta)
  };

  (void)fcntl(fd, F_SETLKW, &flockstr);
}

void DispatchMeta(int fd, char* mapping, void* meta) {
  /* Either meta was locked via fcntl,
   * or it was malloc'ed. */
  if (meta == mapping) {
    UnlockMeta(fd);
  } else {
    free(meta);
  }
}

void UpdateMeta(int fd, char* mapping, off_t new_root, size_t new_height) {
  FileMeta new_meta {
    .root_offset = new_root,
    .height = new_height
  };

  /* WriteLock Meta first */
  WLockMeta(fd, mapping);
  /* Pin Meta in RAM */
  mlock(mapping, sizeof(FileMeta));

  new_meta.order = ((FileMeta*)mapping)->order;

  memcpy(mapping, &new_meta, sizeof(FileMeta));

  /* Unpin Meta from RAM */
  munlock(mapping, sizeof(FileMeta));

  /* Push Meta to disk */
  msync(mapping, sizeof(FileMeta), MS_SYNC);

  UnlockMeta(fd, mapping);
}
