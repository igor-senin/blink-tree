#ifndef FILE_META
#define FILE_META

#include <stddef.h>
#include <unistd.h>

struct FileMeta {
  off_t root_offset;
  size_t height;
} __attribute__((packed));


bool RLockMeta(int fd);

FileMeta* GetMetaLocked(int fd, char* mapping);

void WLockMeta(int fd);

void UnlockMeta(int fd);

void DispatchMeta(int fd, char* mapping, void* meta);

#endif  // FILE_META
