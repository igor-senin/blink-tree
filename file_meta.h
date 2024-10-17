#ifndef FILE_META
#define FILE_META

#include <stddef.h>
#include <unistd.h>

struct FileMeta {
  off_t root_offset;
  size_t height;

  char pad[4096 - sizeof(off_t) - sizeof(size_t)];
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
