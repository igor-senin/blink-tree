#include "blinktree.hpp"

#include <cstdlib>
#include <cstring>

char record[4096];

int main() {
  std::string path = "./database/test_2.bin";

  BLinkTree<4> tree{path};

  char source[] = "_ABRA_CADA_BRA!_";
  for (int i = 0; i < 4096 / 16; ++i) {
    std::memcpy(record + i*16, source, 16);
  }


  for (int i = 0; i < 10; ++i) {
    tree.Insert(((uint64_t)rand() << 32) + (uint64_t)rand(), record);
  }
}
