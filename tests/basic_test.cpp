#include <iostream>
#include <cassert>

// Basic test framework
int tests_passed = 0;
int tests_failed = 0;

void test_assert(bool condition, const char* test_name) {
    if (condition) {
        std::cout << "✓ " << test_name << std::endl;
        tests_passed++;
    } else {
        std::cerr << "✗ " << test_name << " FAILED" << std::endl;
        tests_failed++;
    }
}

// Basic functionality tests
void test_basic_sanity() {
    test_assert(true, "Sanity check");
    test_assert(1 + 1 == 2, "Basic arithmetic");
}

void test_string_operations() {
    std::string test = "hyprbar";
    test_assert(test.length() == 7, "String length check");
    test_assert(test.substr(0, 4) == "hypr", "String substring check");
}

int main() {
    std::cout << "Running Hyprbar Tests..." << std::endl;
    std::cout << "========================" << std::endl;

    test_basic_sanity();
    test_string_operations();

    std::cout << "========================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;

    return tests_failed == 0 ? 0 : 1;
}
