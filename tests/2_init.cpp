#include "touch_file.hpp"

int main(int argc, char* argv[]) {

  std::string path = "./database/test_2.bin";

  TouchTreeFile<4>(path);

  return 0;
}
