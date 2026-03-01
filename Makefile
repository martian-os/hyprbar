# Hyprbar Makefile

# Compiler and flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS = -lwayland-client -lcairo -lpangocairo-1.0 -lpango-1.0

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests
INCLUDE_DIR = include

# Files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET = $(BIN_DIR)/hyprbar

# Test files
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.cpp)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.cpp=$(BUILD_DIR)/test_%.o)
TEST_TARGET = $(BIN_DIR)/test_hyprbar

# Phony targets
.PHONY: all clean test install uninstall dirs

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

# Test target
test: dirs $(TEST_TARGET)
	@echo "Running tests..."
	@$(TEST_TARGET)

# Build test executable
$(TEST_TARGET): $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS))
	@echo "Linking test executable..."
	@$(CXX) $(TEST_OBJECTS) $(filter-out $(BUILD_DIR)/main.o,$(OBJECTS)) -o $(TEST_TARGET) $(LDFLAGS)

# Compile test files
$(BUILD_DIR)/test_%.o: $(TEST_DIR)/%.cpp
	@echo "Compiling test $<..."
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
debug: CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -g -O0
debug: clean all

# Release build (optimized)
release: CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O3 -DNDEBUG
release: clean all

# Show help
help:
	@echo "Hyprbar Makefile targets:"
	@echo "  all       - Build the project (default)"
	@echo "  test      - Build and run tests"
	@echo "  clean     - Remove build artifacts"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release version"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help      - Show this help message"
