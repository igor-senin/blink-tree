#ifndef FILE_META
#define FILE_META

#include <stddef.h>
#include <unistd.h>

struct FileMeta {
  off_t root_offset; /* current root of tree; may be overwritten */
  size_t height; /* current height of tree */
  size_t order; /* 2*order is max number of elements in node */

  char pad[4096 - sizeof(root_offset) - sizeof(height) - sizeof(order)];
} __attribute__((packed));


bool RLockMeta(int fd);

FileMeta* GetMetaLocked(int fd, char* mapping);

void WLockMeta(int fd);

void UnlockMeta(int fd);

void DispatchMeta(int fd, char* mapping, void* meta);

void UpdateMeta(int fd,
                char* mapping,
                off_t new_root,
                size_t new_height);

#endif  // FILE_META
