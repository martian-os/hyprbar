# Hyprbar Makefile

# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude $(shell pkg-config --cflags pangocairo dbus-1 gdk-pixbuf-2.0)
LDFLAGS = -lwayland-client -lcairo -ldbus-1 $(shell pkg-config --libs pangocairo gdk-pixbuf-2.0)

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests
INCLUDE_DIR = include
PROTOCOL_DIR = protocol

# Files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
PROTOCOL_SOURCES = $(wildcard $(PROTOCOL_DIR)/*.c)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES)) \
          $(patsubst $(PROTOCOL_DIR)/%.c,$(BUILD_DIR)/protocol_%.o,$(PROTOCOL_SOURCES))
TARGET = $(BIN_DIR)/hyprbar

# Test files
TEST_SOURCES = $(filter-out $(TEST_DIR)/example_mock_test.cpp $(TEST_DIR)/test_mocks.cpp, $(wildcard $(TEST_DIR)/*.cpp))
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/test_%.o) $(BUILD_DIR)/test_mocks.o
TEST_TARGET = $(BIN_DIR)/test_hyprbar

# Phony targets
.PHONY: all clean test test-fast test-integration test-mocks install uninstall dirs

# Default target
all: dirs $(TARGET)

# Create necessary directories
dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

# Build main executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	@$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files (handle subdirectories)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile protocol C files with C compiler to avoid unused warnings
$(BUILD_DIR)/protocol_%.o: $(PROTOCOL_DIR)/%.c
	@echo "Compiling protocol $<..."
	@mkdir -p $(dir $@)
	@gcc -fPIC -c $< -o $@

# Test target - fast unit tests only (no mocks, for pre-commit)
test-fast: dirs $(TEST_TARGET)
	@echo "Running fast unit tests (no mocks)..."
	@./tests/test_root_clean.sh
	@HYPRBAR_TEST_MODE=fast $(TEST_TARGET)

# Test with mocks - full integration tests
test-mocks: dirs $(TEST_TARGET)
	@echo "Running tests with mock services..."
	@HYPRBAR_TEST_MODE=mocks $(TEST_TARGET)

# Full test suite (both fast and mocks)
test: test-fast

# Build test executable
$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	@echo "Linking test executable..."
	@$(CXX) $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS)) -o $(TEST_TARGET) $(LDFLAGS)

# Compile test files
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.cpp
	@echo "Compiling test $<..."
	@$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

# Compile test mocks (not following test_* pattern)
$(BUILD_DIR)/test_mocks.o: $(TEST_DIR)/test_mocks.cpp
	@echo "Compiling test mocks..."
	@$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -c $< -o $@

# Install target
install: $(TARGET)
	@echo "Installing hyprbar to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall target
uninstall:
	@echo "Removing hyprbar from /usr/local/bin..."
	@sudo rm -f /usr/local/bin/hyprbar
	@echo "Uninstall complete"

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Clean complete"

# Debug build (with debug symbols, no optimization)
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -g -O0 -Iinclude $(shell pkg-config --cflags pangocairo dbus-1 gdk-pixbuf-2.0)
debug: clean all

# Release build (optimized)
release: CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O3 -DNDEBUG -Iinclude $(shell pkg-config --cflags pangocairo dbus-1 gdk-pixbuf-2.0)
release: clean all

# Show help
help:
	@echo "Hyprbar Makefile targets:"
	@echo "  all          - Build the project (default)"
	@echo "  test         - Build and run unit tests"
	@echo "  test-fast    - Run fast unit tests only"
	@echo "  test-asan    - Run tests with AddressSanitizer"
	@echo "  test-tsan    - Run tests with ThreadSanitizer"
	@echo "  integration  - Run integration tests (all configs)"
	@echo "  clean        - Remove build artifacts"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  format       - Auto-format all C++ files"
	@echo "  format-check - Check code formatting"
	@echo "  lint-tidy    - Run clang-tidy static analysis"
	@echo "  lint-cppcheck - Run cppcheck"
	@echo "  quality-fast - Fast quality checks (format + tidy)"
	@echo "  quality-full - Full quality gate (all checks + coverage)"
	@echo "  coverage     - Generate test coverage report"
	@echo "  help         - Show this help message"

# Integration tests (test all example configs)
integration: all
	@echo "Running integration tests..."
	@./tests/integration_test.sh
	@echo "Integration tests complete"

format-check:
	@echo "🔍 Checking code formatting..."
	@FAILED=0; \
	for file in $$(find src tests include -name '*.cpp' -o -name '*.h' 2>/dev/null); do \
		if ! clang-format --dry-run --Werror "$$file" 2>/dev/null; then \
			echo "❌ $$file needs formatting"; \
			FAILED=1; \
		fi; \
	done; \
	if [ $$FAILED -eq 1 ]; then \
		echo ""; \
		echo "Fix with: make format"; \
		exit 1; \
	else \
		echo "✅ All files properly formatted"; \
	fi

format:
	@echo "🔨 Formatting all C++ files..."
	@find src tests include -name '*.cpp' -o -name '*.h' 2>/dev/null | xargs clang-format -i
	@echo "✅ Formatting complete"

lint:
	@echo "Running clang-tidy on all source files..."
	@find src -name "*.cpp" | while read file; do \
		echo "Checking $$file..."; \
		clang-tidy "$$file" -header-filter='include/.*' -- $(CXXFLAGS) 2>&1 | \
		grep -E "(error:|warning:.*function.*too long|warning:.*file.*too long)" || true; \
	done
	@echo "Lint complete"

# Code coverage
.PHONY: coverage coverage-report coverage-clean

coverage: CXXFLAGS += --coverage -O0 -g
coverage: LDFLAGS += --coverage  
coverage: clean dirs
	@echo "Building with coverage instrumentation..."
	@$(MAKE) test CXXFLAGS="$(CXXFLAGS)" LDFLAGS="$(LDFLAGS)"
	@echo "Running tests..."
	@$(TEST_TARGET)
	@echo "Generating coverage report..."
	@mkdir -p coverage
	@geninfo build --output-file coverage/coverage.info --rc branch_coverage=1 --ignore-errors mismatch,inconsistent 2>/dev/null || true
	@lcov --remove coverage/coverage.info '/usr/*' '*/tests/*' --output-file coverage/coverage.info --ignore-errors inconsistent 2>/dev/null || true
	@echo ""
	@echo "Coverage Report"
	@echo "==============="
	@lcov --summary coverage/coverage.info --ignore-errors inconsistent 2>&1 | grep -A 3 "Summary"
	@echo ""
	@echo "For detailed report: make coverage-report"

coverage-report: coverage
	@echo "Generating HTML coverage report..."
	@genhtml coverage/coverage.info --output-directory coverage/html --rc branch_coverage=1 --ignore-errors inconsistent 2>/dev/null
	@echo "Coverage report: coverage/html/index.html"

coverage-clean:
	@rm -rf coverage
	@find $(BUILD_DIR) -name '*.gcda' -o -name '*.gcno' -delete 2>/dev/null || true
	@echo "Coverage data cleaned"

# Static analysis
.PHONY: lint-tidy lint-cppcheck quality-fast quality-full

lint-tidy:
	@echo "🔍 Running clang-tidy..."
	@find src tests -name '*.cpp' | xargs clang-tidy --config-file=.clang-tidy \
		-- -std=c++17 -I include $(shell pkg-config --cflags wayland-client cairo pangocairo dbus-1 gdk-pixbuf-2.0)

lint-cppcheck:
	@echo "🔍 Running cppcheck..."
	@cppcheck --enable=all --inconclusive --std=c++17 \
		--suppress=missingIncludeSystem \
		--suppress=unusedFunction \
		--suppress=unmatchedSuppression \
		--inline-suppr \
		--error-exitcode=1 \
		-I include src/

quality-fast: format-check lint-tidy
	@echo "✅ Fast quality checks passed"

quality-full: format-check lint-tidy lint-cppcheck test-asan coverage
	@echo "✅ Full quality gate passed"

# Memory safety tests
.PHONY: test-asan test-tsan

ASAN_FLAGS = -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -g -O1
TSAN_FLAGS = -fsanitize=thread -g -O1

test-asan:
	@echo "🧪 Building and running tests with AddressSanitizer..."
	@$(MAKE) clean
	@$(MAKE) dirs
	@CXXFLAGS="$(CXXFLAGS) $(ASAN_FLAGS)" LDFLAGS="$(LDFLAGS) $(ASAN_FLAGS)" $(MAKE) $(TEST_TARGET)
	@ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 $(TEST_TARGET)
	@echo "✅ AddressSanitizer tests passed"

test-tsan:
	@echo "🧪 Building and running tests with ThreadSanitizer..."
	@$(MAKE) clean
	@$(MAKE) dirs
	@CXXFLAGS="$(CXXFLAGS) $(TSAN_FLAGS)" LDFLAGS="$(LDFLAGS) $(TSAN_FLAGS)" $(MAKE) $(TEST_TARGET)
	@TSAN_OPTIONS=halt_on_error=1 $(TEST_TARGET)
	@echo "✅ ThreadSanitizer tests passed"
