#include "touch_file.hpp"

#include <string>

int main(int argc, char* argv[]) {

  std::string path = "./database/test_1.bin";

  TouchTreeFile<4>(path);

  return 0;
}
