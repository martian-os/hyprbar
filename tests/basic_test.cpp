#include "test_utils.h"
#include <chrono>
#include <string>
#include <thread>

void test_basic_sanity() {
  test::assert(true, "Sanity check");
  test::assert(1 + 1 == 2, "Basic arithmetic");
}

void test_string_operations() {
  std::string test = "hyprbar";
  test::assert(test.length() == 7, "String length check");
  test::assert(test.substr(0, 4) == "hypr", "String substring check");
}

void test_chrono_duration() {
  using namespace std::chrono;
  auto duration = milliseconds(1000);
  auto seconds_count = duration_cast<seconds>(duration).count();
  test::assert(seconds_count == 1, "Duration conversion");
}

void run_basic_tests() {
  std::cout << "\n--- Basic Tests ---" << std::endl;
  test_basic_sanity();
  test_string_operations();
  test_chrono_duration();
}
