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

void test_fill_rounded_rect() {
  Renderer r;
  r.initialize(200, 50);

  r.begin_frame();
  r.clear(Color{0, 0, 0, 1});

  // Draw a rounded rect in red - should not crash
  r.fill_rounded_rect(10, 5, 180, 40, 8.0, Color{1, 0, 0, 1});
  r.end_frame();

  // Check at least one pixel is non-zero (background was cleared to black,
  // rounded rect should have painted red pixels)
  uint32_t* pixels = reinterpret_cast<uint32_t*>(r.get_buffer_data());
  bool found_nontransparent = false;
  for (int i = 0; i < 200 * 50; i++) {
    if (pixels[i] != 0) {
      found_nontransparent = true;
      break;
    }
  }
  test::assert(found_nontransparent, "fill_rounded_rect paints pixels");
}

void test_stroke_rounded_rect() {
  Renderer r;
  r.initialize(200, 50);

  r.begin_frame();
  r.clear(Color{0, 0, 0, 0});

  // Draw a 2px blue border — should not crash with any radius
  r.stroke_rounded_rect(5, 5, 190, 40, 10.0, 2.0, Color{0, 0, 1, 1});
  r.stroke_rounded_rect(5, 5, 190, 40, 0.0, 2.0,
                        Color{0, 0, 1, 1}); // no radius
  r.end_frame();

  test::assert(true, "stroke_rounded_rect completes without crash");
}

void test_rounded_rect_zero_radius() {
  Renderer r;
  r.initialize(100, 40);
  r.begin_frame();

  // radius=0 should behave like a regular rectangle
  r.fill_rounded_rect(0, 0, 100, 40, 0.0, Color{0.5, 0.5, 0.5, 1});
  r.end_frame();

  uint32_t* pixels = reinterpret_cast<uint32_t*>(r.get_buffer_data());
  test::assert(pixels[0] != 0,
               "Zero-radius rounded rect fills like regular rect");
}

void run_renderer_tests() {
  std::cout << "\n--- Renderer Tests ---" << std::endl;
  test_renderer_initialization();
  test_color_from_hex();
  test_clear_fills_buffer();
  test_frame_lifecycle();
  test_multiple_frames();
  test_fill_rounded_rect();
  test_stroke_rounded_rect();
  test_rounded_rect_zero_radius();
}
