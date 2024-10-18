#include "touch_file.hpp"
#include "blinktree.hpp"

int main(int argc, char* argv[]) {

  std::string path = "./database/test_0.bin";

  TouchTreeFile<8>(path);

  ReadBinFile<8>(path);

  BLinkTree<8> tree{path};

  tree.Insert(5, "ABRACADABRA");

  ReadBinFile<8>(path);

  return 0;
}
