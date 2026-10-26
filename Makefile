# ---------------------------------------------------------
#  Simple Makefile - Library Management System
#  Folders: src/*.c   include/library.h   tests/*.cpp
#  Commands: make | make test | make valgrind | make coverage
#            make check | make misra | make clean
# ---------------------------------------------------------

CC       = gcc
CXX      = g++
CFLAGS   = -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude -Wall -Wextra -g -O0
CXXFLAGS = -std=c++17 -Iinclude -Itests -Wall -Wextra -g -O0
EXTRA    =                  # "make coverage" fills this with --coverage

SRC      = $(wildcard src/*.c)                    # all C files
LIB_SRC  = $(filter-out src/main.c,$(SRC))        # all C files except main.c
TESTS    = $(wildcard tests/*.cpp)                # GoogleTest files
OBJ      = $(patsubst src/%.c,build/%.o,$(LIB_SRC))

.PHONY: all test valgrind coverage check misra clean

# 1) Build the program:  make
all: library_app

library_app: $(OBJ) build/main.o
	$(CC) $(EXTRA) -o $@ $^

# Turn each src/x.c into build/x.o
build/%.o: src/%.c include/library.h
	mkdir -p build
	$(CC) $(CFLAGS) $(EXTRA) -c $< -o $@

# 2) Unit tests (GoogleTest):  make test
build/tests: $(TESTS) tests/test_helpers.h $(OBJ)
	$(CXX) $(CXXFLAGS) $(EXTRA) -o $@ $(TESTS) $(OBJ) -lgtest_main -lgtest -pthread

test: build/tests
	./build/tests

# 3) Memory check:  make valgrind
valgrind: build/tests
	valgrind --leak-check=full --error-exitcode=1 ./build/tests

# 4) Code coverage:  make coverage
coverage: clean
	$(MAKE) EXTRA=--coverage test
	gcov -b -c -o build $(LIB_SRC) | grep -A1 "^File 'src/"

# 5) Static analysis:  make check
check:
	cppcheck --enable=warning,performance --suppress=missingIncludeSystem \
	         --std=c11 --language=c --error-exitcode=1 -Iinclude src

# 6) MISRA C check (cppcheck addon, never stops the build):  make misra
misra:
	-cppcheck --addon=misra --suppress=missingIncludeSystem \
	          --std=c11 --language=c -Iinclude src

# 7) Delete everything built:  make clean
clean:
	rm -rf build library_app *.gcov *.gcda *.gcno
