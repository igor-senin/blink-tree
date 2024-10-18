all: test_1

CXX = g++
CXXFLAGS = -Wall -c -std=c++23 -I.

BUILDDIR = ./build/
BINDIR = ./bin/
TESTS = ./tests/
MKDIR = mkdir -p $(BINDIR) $(BUILDDIR)

OBJFILES = $(BUILDDIR)alloc.o \
					 $(BUILDDIR)allocc.o \
					 $(BUILDDIR)meta.o

test_1: alloc meta
	$(CXX) $(CXXFLAGS) $(TESTS)1_init.cpp -o $(BUILDDIR)1_init.o
	$(CXX) $(CXXFLAGS) $(TESTS)1_main.cpp -o $(BUILDDIR)1_main.o
	$(CXX) $(CXXFLAGS) $(TESTS)1_print.cpp -o $(BUILDDIR)1_print.o

	$(CXX) $(OBJFILES) $(BUILDDIR)1_init.o -o $(BINDIR)1_init
	$(CXX) $(OBJFILES) $(BUILDDIR)1_main.o -o $(BINDIR)1_main
	$(CXX) $(OBJFILES) $(BUILDDIR)1_print.o -o $(BINDIR)1_print

	$(CXX) $(TESTS)1_tester.c -o $(BINDIR)1_tester

test_2: alloc meta
	$(CXX) $(CXXFLAGS) $(TESTS)2_init.cpp -o $(BUILDDIR)2_init.o
	$(CXX) $(CXXFLAGS) $(TESTS)2_main.cpp -o $(BUILDDIR)2_main.o
	$(CXX) $(CXXFLAGS) $(TESTS)2_print.cpp -o $(BUILDDIR)2_print.o

	$(CXX) $(OBJFILES) $(BUILDDIR)2_init.o -o $(BINDIR)2_init
	$(CXX) $(OBJFILES) $(BUILDDIR)2_main.o -o $(BINDIR)2_main
	$(CXX) $(OBJFILES) $(BUILDDIR)2_print.o -o $(BINDIR)2_print

	$(CXX) $(TESTS)2_tester.c -o $(BINDIR)2_tester

ALLOC_TARGETS := $(wildcard allocate*)
alloc: $(ALLOC_TARGETS) outputdir
	$(CXX) $(CXXFLAGS) allocate.c -o $(BUILDDIR)allocc.o
	$(CXX) $(CXXFLAGS) allocate_data.cpp -o $(BUILDDIR)alloc.o

META_TARGETS := $(wildcard file_meta*)
meta: $(META_TARGETS) outputdir
	$(CXX) $(CXXFLAGS) file_meta.c -o $(BUILDDIR)meta.o

outputdir:
	$(MKDIR)

clean:
	rm -rf $(BUILDDIR)
	rm -rf $(BINDIR)
	rm -rf database/
	
.PHONY: clean
