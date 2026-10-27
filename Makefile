# --- Compilers and Flags ---
CC       = gcc
CXX      = g++
CFLAGS   = -g -Wall --coverage -Iinclude -I.
CXXFLAGS = -std=c++17 -g -Wall --coverage -Iinclude -I.
LDFLAGS  = --coverage -lgtest -lgtest_main -pthread -lstdc++fs

# --- Source Files ---
# All C files EXCEPT main.c for the tests
SRC_C = book.c \
        member.c \
        storage.c \
        transaction.c \
        utils.c

OBJ       = $(SRC_C:.c=.o)
TEST_SRC  = test_library.cpp
TEST_BIN  = run_all_tests
MAIN_BIN  = library_app

.PHONY: all test cppcheck valgrind run clean audit

all: test

# 1. Compile the C files for testing
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 2. Link the Test Executable
$(TEST_BIN): $(OBJ) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(TEST_SRC) $(OBJ) $(LDFLAGS) -o $(TEST_BIN)

# 3. Run the Tests
test: $(TEST_BIN)
	./$(TEST_BIN)

# 4. Run Cppcheck Static Analysis
cppcheck:
	cppcheck --enable=warning,performance,portability --std=c11 -Iinclude -I. --suppress=missingIncludeSystem . 2> cppcheck_final.txt
	@echo "--- Cppcheck Results ---"
	@cat cppcheck_final.txt

# 5. Compile the Main App and Run Valgrind
valgrind:
	$(CC) $(CFLAGS) *.c -o $(MAIN_BIN)
	valgrind --leak-check=full ./$(MAIN_BIN)

# 6. Compile and Run the Main App Normally
run:
	$(CC) $(CFLAGS) *.c -o $(MAIN_BIN)
	./$(MAIN_BIN)

# 7. Clean up all generated files
clean:
	rm -f *.o *.gcda *.gcno *.gcov $(TEST_BIN) $(MAIN_BIN) cppcheck_final.txt test_input.txt *.dat testing_audit.log

# =========================================================================
# 8. THE AUDIT LOG TARGET (For the instructor)
# =========================================================================
audit:
	@rm -f testing_audit.log
	@echo "==================================================" >> testing_audit.log
	@echo "TIMESTAMP: $$(date)" >> testing_audit.log
	@echo "ACTION: Compilation and GTest Execution" >> testing_audit.log
	@echo "==================================================" >> testing_audit.log
	@$(MAKE) clean >> testing_audit.log 2>&1
	@$(MAKE) test >> testing_audit.log 2>&1
	
	@echo -e "\n==================================================" >> testing_audit.log
	@echo "TIMESTAMP: $$(date)" >> testing_audit.log
	@echo "ACTION: Valgrind Memory Analysis" >> testing_audit.log
	@echo "==================================================" >> testing_audit.log
	@$(CC) $(CFLAGS) *.c -o $(MAIN_BIN)
	@echo 0 | valgrind --leak-check=full ./$(MAIN_BIN) >> testing_audit.log 2>&1
	
	@echo -e "\n==================================================" >> testing_audit.log
	@echo "TIMESTAMP: $$(date)" >> testing_audit.log
	@echo "ACTION: Cppcheck Static Analysis" >> testing_audit.log
	@echo "==================================================" >> testing_audit.log
	@cppcheck --enable=warning,performance,portability --std=c11 -Iinclude -I. --suppress=missingIncludeSystem . >> testing_audit.log 2>&1
	
	@echo "✅ Audit complete! Check 'testing_audit.log' for the timestamped report."
