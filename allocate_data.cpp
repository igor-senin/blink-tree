#include "allocate_data.hpp"


off_t AllocateRecord(int fd, std::string_view record) {
  return Allocate(fd, record.data(), record.size());
}
