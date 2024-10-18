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
  struct flock flockstr = {
    .l_type = F_RDLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = sizeof(struct FileMeta)
  };

  return fcntl(fd, F_SETLK, &flockstr) != -1;
}

struct FileMeta* GetMetaLocked(int fd, char* mapping) {
  if (RLockMeta(fd)) {
    return (struct FileMeta*)mapping;
  }

  size_t size = sizeof(struct FileMeta);
  void* meta = malloc(size);
  if (meta == NULL) {
    perror("malloc");
    return NULL;
  }
  if (pread(fd, meta, size, 0) != size) {
    perror("pread");
    free(meta);
    return NULL;
  }

  return (struct FileMeta*)meta;
}

void WLockMeta(int fd, char* mapping) {
  struct flock flockstr = {
    .l_type = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = sizeof(struct FileMeta)
  };

  (void)fcntl(fd, F_SETLKW, &flockstr);
}

void UnlockMeta(int fd, char* mapping) {
  struct flock flockstr = {
    .l_type = F_UNLCK,
    .l_whence = 0,
    .l_start = 0,
    .l_len = sizeof(struct FileMeta)
  };

  (void)fcntl(fd, F_SETLKW, &flockstr);
}

void DispatchMeta(int fd, char* mapping, void* meta) {
  /* Either meta was locked via fcntl,
   * or it was malloc'ed. */
  if (meta == mapping) {
    UnlockMeta(fd, mapping);
  } else {
    free(meta);
  }
}

void UpdateMeta(int fd, char* mapping, off_t new_root, size_t new_height) {
  struct FileMeta new_meta = {
    .root_offset = new_root,
    .height = new_height
  };

  /* WriteLock Meta first */
  WLockMeta(fd, mapping);
  /* Pin Meta in RAM */
  mlock(mapping, sizeof(struct FileMeta));

  new_meta.order = ((struct FileMeta*)mapping)->order;

  memcpy(mapping, &new_meta, sizeof(struct FileMeta));

  /* Unpin Meta from RAM */
  munlock(mapping, sizeof(struct FileMeta));

  /* Push Meta to disk */
  msync(mapping, sizeof(struct FileMeta), MS_SYNC);

  UnlockMeta(fd, mapping);
}
