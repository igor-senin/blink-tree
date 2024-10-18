#include "allocate.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>


off_t Allocate(int fd, const char* buf, off_t count) {
  struct flock flockstr = {
    .l_type = F_WRLCK,
    .l_whence = SEEK_END,
    .l_start = 0,
    .l_len = count
  };

  // TODO: deal with errors

  if (fcntl(fd, F_SETLKW, &flockstr) == -1) {
    return -1;
  }

  off_t record_offset = lseek(fd, 0, SEEK_END);

  (void)pwrite(fd, buf, count, record_offset);

  flockstr.l_type = F_UNLCK;
  (void)fcntl(fd, F_SETLKW, &flockstr);

  return record_offset;
}
