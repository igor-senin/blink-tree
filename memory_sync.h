#ifndef MEMORY_SYNC_H
#define MEMORY_SYNC_H

#include <stddef.h>
#include <sys/mman.h>

inline void MSync(char* addr, size_t len) {
  msync(addr, len, MS_SYNC);
}

#endif  // MEMORY_SYNC_H
