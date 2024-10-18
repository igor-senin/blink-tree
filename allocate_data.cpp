#include "allocate_data.hpp"

#include "allocate.h"


off_t AllocateRecord(int fd, std::string_view record) {
  // std::cout << "Allocate record: " << record.size() << "\n";
  return Allocate(fd, record.data(), static_cast<off_t>(record.size()));
}
