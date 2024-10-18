#ifndef TOUCH_FILE_HPP
#define TOUCH_FILE_HPP

#include "node.hpp"
#include "file_meta.h"

#include <filesystem>
#include <iostream>
#include <string_view>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>


template <std::size_t KeysCount>
void TouchTreeFile(std::string_view path_view) {
  std::filesystem::path path{path_view};
  std::filesystem::create_directories(path.parent_path());

  FileMeta meta {
    .root_offset = sizeof(FileMeta),
    .height = 1,
    .order = KeysCount
  };

  int fd = open(path_view.data(), O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    std::cerr << "Error in open tree file\n";
    exit(EXIT_FAILURE);
  }

  ssize_t written;

  written = pwrite(fd, &meta, sizeof(meta), 0);

  std::cout << written << "bytes written (meta)\n";

  Node<KeysCount> root;
  root.InitRoot();

  written = pwrite(fd, &root, sizeof(root), sizeof(meta));

  std::cout << written << "bytes written (root)\n";

  struct stat statbuf;
  fstat(fd, &statbuf);

  std::cout << "Current size of file: " << statbuf.st_size << "\n";

  close(fd);
}


template <std::size_t KeysCount>
void ReadRawNode(int fd, off_t node_offset) {
  Node<KeysCount> node;
  (void)pread(fd, &node, sizeof(node), node_offset);

  std::cout << "Node with offset " << node_offset << ": " << "\n";
  std::cout << "\t- node.Level(): " << node.Level() << "\n"
    << "\t- node.Size(): " << node.Size() << "\n";
  node.PrintAll();
}


template <std::size_t KeysCount>
void ReadBinFile(std::string_view path_view) {
  FileMeta meta;

  int fd = open(path_view.data(), O_RDONLY);

  (void)pread(fd, &meta, sizeof(meta), 0);

  Node<KeysCount> root;

  (void)pread(fd, &root, sizeof(root), meta.root_offset);

  std::cout << "Meta:\n\t- meta.root_offset: " << meta.root_offset
    << "\n\t- meta.height: " << meta.height
    << "\n\t- meta.order: " << meta.order
    << "\n\n";

  std::cout << "Root:\n\t- root.Level(): " << root.Level() << "\n";
  std::cout << "\t- root.Size(): " << root.Size() << "\n";
  root.PrintAll();

  for (int i = 0; i < root.Size(); ++i) {
    auto offset = root.GetIthPtr(i);
    ReadRawNode<KeysCount>(fd, offset);
  }

  close(fd);
}

#endif  // TOUCH_FILE_HPP
