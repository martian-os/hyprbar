#include "hyprbar/core/config_manager.h"
#include "hyprbar/core/logger.h"
#include "hyprbar/core/security_validator.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace hyprbar {

// Simple JSON parser (minimal implementation for basic needs)
class JsonParser {
public:
  static std::optional<ConfigValue> parse(const std::string& json) {
    JsonParser parser(json);
    return parser.parse_value();
  }

private:
  JsonParser(const std::string& json) : json_(json), pos_(0) {
  }

  void skip_whitespace() {
    while (pos_ < json_.size() && std::isspace(json_[pos_])) {
      pos_++;
    }
  }

  char peek() {
    skip_whitespace();
    return pos_ < json_.size() ? json_[pos_] : '\0';
  }

  char get() {
    skip_whitespace();
    return pos_ < json_.size() ? json_[pos_++] : '\0';
  }

  bool expect(char ch) {
    if (get() != ch) {
      return false;
    }
    return true;
  }

  std::optional<ConfigValue> parse_value() {
    char ch = peek();

    if (ch == '{') {
      return parse_object();
    } else if (ch == '[') {
      return parse_array();
    } else if (ch == '"') {
      return parse_string();
    } else if (ch == 't' || ch == 'f') {
      return parse_boolean();
    } else if (ch == '-' || std::isdigit(ch)) {
      return parse_number();
    }

    return std::nullopt;
  }

  std::optional<ConfigValue> parse_object() {
    if (!expect('{'))
      return std::nullopt;

    ConfigValue result;
    result.type = ConfigValue::Type::Object;

    if (peek() == '}') {
      get();
      return result;
    }

    while (true) {
      auto key = parse_string();
      if (!key)
        return std::nullopt;

      if (!expect(':'))
        return std::nullopt;

      auto value = parse_value();
      if (!value)
        return std::nullopt;

      result.object_value[key->string_value] = *value;

      char next = peek();
      if (next == '}') {
        get();
        break;
      } else if (next == ',') {
        get();
      } else {
        return std::nullopt;
      }
    }

    return result;
  }

  std::optional<ConfigValue> parse_array() {
    if (!expect('['))
      return std::nullopt;

    ConfigValue result;
    result.type = ConfigValue::Type::Array;

    if (peek() == ']') {
      get();
      return result;
    }

    while (true) {
      auto value = parse_value();
      if (!value)
        return std::nullopt;

      result.array_value.push_back(*value);

      char next = peek();
      if (next == ']') {
        get();
        break;
      } else if (next == ',') {
        get();
      } else {
        return std::nullopt;
      }
    }

    return result;
  }

  std::optional<ConfigValue> parse_string() {
    if (!expect('"'))
      return std::nullopt;

    std::string str;
    while (pos_ < json_.size() && json_[pos_] != '"') {
      if (json_[pos_] == '\\' && pos_ + 1 < json_.size()) {
        pos_++;
        char escaped = json_[pos_++];
        switch (escaped) {
        case 'n':
          str += '\n';
          break;
        case 't':
          str += '\t';
          break;
        case 'r':
          str += '\r';
          break;
        case '"':
          str += '"';
          break;
        case '\\':
          str += '\\';
          break;
        default:
          str += escaped;
          break;
        }
      } else {
        str += json_[pos_++];
      }
    }

    if (!expect('"'))
      return std::nullopt;

    ConfigValue result;
    result.type = ConfigValue::Type::String;
    result.string_value = str;
    return result;
  }

  std::optional<ConfigValue> parse_number() {
    size_t start = pos_;
    bool is_double = false;

    if (json_[pos_] == '-')
      pos_++;

    while (pos_ < json_.size() && std::isdigit(json_[pos_])) {
      pos_++;
    }

    if (pos_ < json_.size() && json_[pos_] == '.') {
      is_double = true;
      pos_++;
      while (pos_ < json_.size() && std::isdigit(json_[pos_])) {
        pos_++;
      }
    }

    std::string num_str = json_.substr(start, pos_ - start);
    ConfigValue result;

    if (is_double) {
      result.type = ConfigValue::Type::Double;
      result.double_value = std::stod(num_str);
    } else {
      result.type = ConfigValue::Type::Integer;
      result.int_value = std::stoll(num_str);
    }

    return result;
  }

  std::optional<ConfigValue> parse_boolean() {
    if (pos_ + 4 <= json_.size() && json_.substr(pos_, 4) == "true") {
      pos_ += 4;
      ConfigValue result;
      result.type = ConfigValue::Type::Boolean;
      result.bool_value = true;
      return result;
    } else if (pos_ + 5 <= json_.size() && json_.substr(pos_, 5) == "false") {
      pos_ += 5;
      ConfigValue result;
      result.type = ConfigValue::Type::Boolean;
      result.bool_value = false;
      return result;
    }
    return std::nullopt;
  }

  std::string json_;
  size_t pos_;
};

std::string ConfigManager::get_default_config_path() {
  const char* home = getenv("HOME");
  if (!home) {
    return "/tmp/hyprbar.json";
  }
  return std::string(home) + "/.config/hyprbar/config.json";
}

bool ConfigManager::load(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    error_ = "Could not open config file: " + path;
    Logger::instance().error(error_);
    return false;
  }

  // Store config directory for relative path resolution
  size_t last_slash = path.find_last_of('/');
  if (last_slash != std::string::npos) {
    config_dir_ = path.substr(0, last_slash);
  } else {
    config_dir_ = ".";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return load_from_string(buffer.str());
}

bool ConfigManager::load_from_string(const std::string& json) {
  return parse_json(json);
}

bool ConfigManager::parse_json(const std::string& json) {
  auto root = JsonParser::parse(json);
  if (!root || !root->is_object()) {
    error_ = "Invalid JSON: root must be an object";
    Logger::instance().error(error_);
    return false;
  }

  // Parse bar config
  auto bar = root->get("bar");
  if (bar && !parse_bar_config(*bar)) {
    return false;
  }

  // Parse widgets
  auto widgets = root->get("widgets");
  if (widgets && !parse_widgets(*widgets)) {
    return false;
  }

  Logger::instance().info("Configuration loaded successfully");
  return true;
}

bool ConfigManager::parse_bar_config(const ConfigValue& value) {
  if (!value.is_object()) {
    error_ = "Bar config must be an object";
    return false;
  }

  if (auto pos = value.get("position")) {
    config_.bar.position = parse_position(pos->as_string());
  }

  if (auto height = value.get("height")) {
    config_.bar.height = static_cast<uint32_t>(height->as_int());
  }

  if (auto bg = value.get("background")) {
    config_.bar.background = bg->as_string();
  }

  if (auto color = value.get("color")) {
    config_.bar.color = color->as_string();
  }

  if (auto font = value.get("font")) {
    config_.bar.font = font->as_string();
  }

  if (auto size = value.get("size")) {
    if (size->type == ConfigValue::Type::Integer) {
      config_.bar.size = static_cast<double>(size->as_int());
    } else if (size->type == ConfigValue::Type::Double) {
      config_.bar.size = size->as_double();
    }
  }

  if (auto margin = value.get("margin")) {
    config_.bar.margin = static_cast<int>(margin->as_int());
  }

  if (auto gap = value.get("gap")) {
    config_.bar.gap = static_cast<int>(gap->as_int());
  }

  if (auto padding = value.get("padding")) {
    config_.bar.padding = static_cast<int>(padding->as_int());
  }

  if (auto br = value.get("border-radius")) {
    config_.bar.border_radius = static_cast<int>(br->as_int());
  }

  if (auto bc = value.get("border-color")) {
    config_.bar.border_color = bc->as_string();
  }

  if (auto bw = value.get("border-width")) {
    config_.bar.border_width = static_cast<int>(bw->as_int());
  }

  return true;
}

bool ConfigManager::parse_widgets(const ConfigValue& value) {
  if (!value.is_array()) {
    error_ = "Widgets must be an array";
    return false;
  }

  for (const auto& widget_val : value.array_value) {
    if (!widget_val.is_object()) {
      error_ = "Each widget must be an object";
      return false;
    }

    WidgetConfig widget;

    auto type = widget_val.get("type");
    if (!type) {
      error_ = "Widget missing 'type' field";
      return false;
    }
    widget.type = type->as_string();

    if (auto pos = widget_val.get("position")) {
      widget.position = parse_widget_position(pos->as_string());
    }

    if (auto cfg = widget_val.get("config")) {
      widget.config = *cfg;
    }

    // Parse CSS-like widget style from top-level widget object or "style"
    // sub-object Supports both inline: {"background": "#..."} and nested:
    // {"style": {"background": "#..."}}
    auto parse_widget_style = [](const ConfigValue& src, WidgetStyle& style) {
      if (auto bg = src.get("background")) {
        style.background = bg->as_string();
      }
      if (auto p = src.get("padding")) {
        style.padding = static_cast<int>(p->as_int());
      }
      if (auto br = src.get("border-radius")) {
        style.border_radius = static_cast<int>(br->as_int());
      }
      if (auto bc = src.get("border-color")) {
        style.border_color = bc->as_string();
      }
      if (auto bw = src.get("border-width")) {
        style.border_width = static_cast<int>(bw->as_int());
      }
    };

    // Check top-level widget object first
    parse_widget_style(widget_val, widget.style);

    // "style" sub-object takes precedence (more explicit)
    if (auto style_obj = widget_val.get("style")) {
      if (style_obj->is_object()) {
        parse_widget_style(*style_obj, widget.style);
      }
    }

    config_.widgets.push_back(widget);
  }

  return true;
}

BarConfig::Position ConfigManager::parse_position(const std::string& pos) {
  std::string lower = pos;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == "top")
    return BarConfig::Position::Top;
  if (lower == "bottom")
    return BarConfig::Position::Bottom;
  if (lower == "left")
    return BarConfig::Position::Left;
  if (lower == "right")
    return BarConfig::Position::Right;

  return BarConfig::Position::Top; // default
}

WidgetConfig::Position
ConfigManager::parse_widget_position(const std::string& pos) {
  std::string lower = pos;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

  if (lower == "left")
    return WidgetConfig::Position::Left;
  if (lower == "center")
    return WidgetConfig::Position::Center;
  if (lower == "right")
    return WidgetConfig::Position::Right;

  return WidgetConfig::Position::Left; // default
}

std::string ConfigManager::resolve_path(const std::string& path) const {
  if (path.empty()) {
    return path;
  }

  // Absolute path - return as-is
  if (path[0] == '/') {
    return path;
  }

  // Home directory expansion
  if (path[0] == '~') {
    const char* home = getenv("HOME");
    if (!home) {
      return path;
    }
    if (path.length() == 1) {
      return std::string(home);
    }
    if (path[1] == '/') {
      return std::string(home) + path.substr(1);
    }
    return path; // ~user syntax not supported
  }

  // Relative path - resolve relative to config directory
  std::filesystem::path base_dir = config_dir_.empty() || config_dir_ == "."
                                       ? std::filesystem::current_path()
                                       : std::filesystem::path(config_dir_);

  std::filesystem::path resolved = base_dir / path;

  // Canonicalize to resolve .. and symlinks
  try {
    resolved = std::filesystem::canonical(resolved);
    return resolved.string();
  } catch (const std::filesystem::filesystem_error& e) {
    Logger::instance().warn("Path resolution failed for '{}': {}", path,
                            e.what());
    // If canonical fails (file doesn't exist), return the absolute path anyway
    return resolved.string();
  }
}

} // namespace hyprbar
