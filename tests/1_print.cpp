#include "touch_file.hpp"

int main() {
  std::string path = "./database/test_1.bin";

  ReadBinFile<4>(path);

  return 0;
}
