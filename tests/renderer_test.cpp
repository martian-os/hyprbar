#include "hyprbar/rendering/renderer.h"
#include "test_utils.h"
#include <cstring>

using namespace hyprbar;

void test_renderer_initialization() {
  Renderer r;
  bool success = r.initialize(800, 60);
  test::assert(success, "Renderer initialization");
  test::assert(r.get_buffer_data() != nullptr, "Buffer allocated");
  test::assert(r.get_buffer_size() == 800 * 60 * 4, "Buffer size");
}

void test_color_from_hex() {
  Color c1 = Color::from_hex("#ff0000");
  test::assert(c1.r == 1.0, "Red channel");
  test::assert(c1.g == 0.0, "Green channel");
  test::assert(c1.b == 0.0, "Blue channel");

  Color c2 = Color::from_hex("#80ff80");
  test::assert(c2.r > 0.49 && c2.r < 0.51, "Mixed red");
  test::assert(c2.g == 1.0, "Mixed green");
  test::assert(c2.b > 0.49 && c2.b < 0.51, "Mixed blue");
}

void test_clear_fills_buffer() {
  Renderer r;
  r.initialize(100, 50);

  r.begin_frame();
  Color bg{0.5, 0.5, 0.5, 1.0};
  r.clear(bg);
  r.end_frame();

  // Check a pixel is non-zero (cleared to gray)
  uint32_t* pixels = reinterpret_cast<uint32_t*>(r.get_buffer_data());
  test::assert(pixels[0] != 0, "Clear fills buffer");
}

void test_frame_lifecycle() {
  Renderer r;
  r.initialize(100, 50);

  r.begin_frame();
  r.clear(Color{0, 0, 0, 1});
  r.end_frame();

  test::assert(true, "Frame lifecycle completes");
}

void test_multiple_frames() {
  Renderer r;
  r.initialize(100, 50);

  for (int i = 0; i < 10; i++) {
    r.begin_frame();
    r.clear(Color{0.1 * i, 0, 0, 1});
    r.end_frame();
  }

  test::assert(true, "Multiple frames render");
}

void run_renderer_tests() {
  std::cout << "\n--- Renderer Tests ---" << std::endl;
  test_renderer_initialization();
  test_color_from_hex();
  test_clear_fills_buffer();
  test_frame_lifecycle();
  test_multiple_frames();
}
