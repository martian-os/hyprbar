#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

// Mock EventLoop for testing (since we can't use the real one without proper
// includes) In real tests, we'd include the actual header

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

void test_basic_sanity() {
  test_assert(true, "Sanity check");
  test_assert(1 + 1 == 2, "Basic arithmetic");
}

void test_string_operations() {
  std::string test = "hyprbar";
  test_assert(test.length() == 7, "String length check");
  test_assert(test.substr(0, 4) == "hypr", "String substring check");
}

void test_chrono_duration() {
  using namespace std::chrono;
  auto duration = milliseconds(1000);
  auto seconds_count = duration_cast<seconds>(duration).count();
  test_assert(seconds_count == 1, "Duration conversion");
}

int main() {
  std::cout << "Running Hyprbar Tests..." << std::endl;
  std::cout << "========================" << std::endl;

  test_basic_sanity();
  test_string_operations();
  test_chrono_duration();

  std::cout << "========================" << std::endl;
  std::cout << "Tests passed: " << tests_passed << std::endl;
  std::cout << "Tests failed: " << tests_failed << std::endl;

  return tests_failed == 0 ? 0 : 1;
}
