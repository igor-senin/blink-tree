#ifndef FILE_META
#define FILE_META

#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>

struct FileMeta {
  off_t root_offset; /* current root of tree; may be overwritten */
  size_t height; /* current height of tree */
  size_t order; /* 2*order is max number of elements in node */

  char pad[4096 - sizeof(off_t) - sizeof(size_t) - sizeof(size_t)];
} __attribute__((packed));


bool RLockMeta(int fd);

struct FileMeta* GetMetaLocked(int fd, char* mapping);

void WLockMeta(int fd, char* mapping);

void UnlockMeta(int fd, char* mapping);

void DispatchMeta(int fd, char* mapping, void* meta);

void UpdateMeta(int fd,
                char* mapping,
                off_t new_root,
                size_t new_height);

#endif  // FILE_META
