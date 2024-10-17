#ifndef ALLOCATE_H
#define ALLOCATE_H

#include <sys/types.h>

off_t Allocate(int fd, const char* buf, off_t count);

#endif  // ALLOCATE_H
