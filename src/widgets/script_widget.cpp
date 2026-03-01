#include "hyprbar/widgets/script_widget.h"
#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/rendering/renderer.h"
#include <array>
#include <cstdio>
#include <memory>

namespace hyprbar {

bool ScriptWidget::initialize(const ConfigValue& config) {
  if (config.type != ConfigValue::Type::Object) {
    Logger::instance().error("Script widget requires configuration object");
    return false;
  }

  const auto& obj = config.object_value;

  if (!obj.count("command")) {
    Logger::instance().error("Script widget requires 'command' parameter");
    return false;
  }

  command_ = obj.at("command").string_value;

  if (obj.count("interval")) {
    const auto& interval_val = obj.at("interval");
    if (interval_val.type == ConfigValue::Type::Integer) {
      interval_ms_ = static_cast<int>(interval_val.int_value);
    } else if (interval_val.type == ConfigValue::Type::Double) {
      interval_ms_ = static_cast<int>(interval_val.double_value);
    }
  }

  if (obj.count("font")) {
    font_ = obj.at("font").string_value;
  }

  if (obj.count("size")) {
    const auto& size_val = obj.at("size");
    if (size_val.type == ConfigValue::Type::Integer) {
      font_size_ = static_cast<double>(size_val.int_value);
    } else if (size_val.type == ConfigValue::Type::Double) {
      font_size_ = size_val.double_value;
    }
  }

  if (obj.count("color")) {
    color_ = obj.at("color").string_value;
  }

  last_update_ = std::chrono::steady_clock::now();
  last_output_ = execute_command();

  Logger::instance().debug("Script widget initialized: {}", command_);
  return true;
}

std::string ScriptWidget::execute_command() {
  std::array<char, 256> buffer;
  std::string result;
  std::string last_line;

  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command_.c_str(), "r"),
                                                pclose);

  if (!pipe) {
    Logger::instance().error("Failed to execute command: {}", command_);
    return "ERROR";
  }

  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result = buffer.data();
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
      result.pop_back();
    }
    if (!result.empty()) {
      last_line = result;
    }
  }

  return last_line.empty() ? "" : last_line;
}

bool ScriptWidget::update() {
  auto now = std::chrono::steady_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update_);

  if (elapsed.count() >= interval_ms_) {
    std::string new_output = execute_command();
    last_update_ = now;

    if (new_output != last_output_) {
      last_output_ = new_output;
      return true; // Needs redraw
    }
  }

  return false;
}

void ScriptWidget::render(Renderer& renderer, int x, int y, int /*width*/,
                          int height) {
  Color fg = Color::from_hex(color_);
  double text_y = y + (height / 2.0) + (font_size_ / 3.0);
  renderer.draw_text(last_output_, x, text_y, font_, font_size_, fg);
}

int ScriptWidget::get_desired_width() const {
  // Estimate: ~8 pixels per character
  return static_cast<int>(last_output_.length() * 8 + 20);
}

int ScriptWidget::get_desired_height() const {
  return 0; // Flexible height
}

} // namespace hyprbar
