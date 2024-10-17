all: btree

CXX = g++
CC = gcc
CXXFLAGS = -Wall -std=c++23 -I.
CCFLAGS = -Wall -I.

OUTPUTDIR = ./bin/
MKDIR = mkdir -p $(OUTPUTDIR)

BTREE_TARGETS = blinktree.hpp node.hpp locks.hpp memory_sync.h
btree: alloc meta $(BTREE_TARGETS)

ALLOC_TARGETS := $(wildcard allocate*)
alloc: $(ALLOC_TARGETS) outputdir
	$(CC) $(CCFLAGS) allocate.c -o $(OUTPUTDIR)allocc.o
	$(CXX) $(CXXFLAGS) allocate_data.cpp allocc.o -o $(OUTPUTDIR)alloc.o

META_TARGETS := $(wildcard file_meta*)
meta: $(META_TARGETS) outputdir
	$(CC) $(CCFLAGS) file_meta.c -o $(OUTPUTDIR)meta.o

outputdir:
	$(MKDIR)

clean:
	rm -rf $(OUTPUTDIR)
	
.PHONY: clean tree
