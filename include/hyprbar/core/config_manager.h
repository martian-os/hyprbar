#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace hyprbar {

/**
 * Configuration value - can be string, int, double, bool, or nested object
 */
struct ConfigValue {
  enum class Type { String, Integer, Double, Boolean, Object, Array };

  Type type;
  std::string string_value;
  int64_t int_value;
  double double_value;
  bool bool_value;
  std::map<std::string, ConfigValue> object_value;
  std::vector<ConfigValue> array_value;

  ConfigValue()
      : type(Type::String), string_value(), int_value(0), double_value(0.0),
        bool_value(false), object_value(), array_value() {
  }

  explicit ConfigValue(const std::string& s)
      : type(Type::String), string_value(s), int_value(0), double_value(0.0),
        bool_value(false), object_value(), array_value() {
  }
  explicit ConfigValue(const char* s)
      : type(Type::String), string_value(s), int_value(0), double_value(0.0),
        bool_value(false), object_value(), array_value() {
  }
  explicit ConfigValue(int64_t i)
      : type(Type::Integer), string_value(), int_value(i), double_value(0.0),
        bool_value(false), object_value(), array_value() {
  }
  explicit ConfigValue(double d)
      : type(Type::Double), string_value(), int_value(0), double_value(d),
        bool_value(false), object_value(), array_value() {
  }
  explicit ConfigValue(bool b)
      : type(Type::Boolean), string_value(), int_value(0), double_value(0.0),
        bool_value(b), object_value(), array_value() {
  }
  explicit ConfigValue(const std::map<std::string, ConfigValue>& obj)
      : type(Type::Object), string_value(), int_value(0), double_value(0.0),
        bool_value(false), object_value(obj), array_value() {
  }
  explicit ConfigValue(const std::vector<ConfigValue>& arr)
      : type(Type::Array), string_value(), int_value(0), double_value(0.0),
        bool_value(false), object_value(), array_value(arr) {
  }

  bool is_string() const {
    return type == Type::String;
  }
  bool is_int() const {
    return type == Type::Integer;
  }
  bool is_double() const {
    return type == Type::Double;
  }
  bool is_bool() const {
    return type == Type::Boolean;
  }
  bool is_object() const {
    return type == Type::Object;
  }
  bool is_array() const {
    return type == Type::Array;
  }

  std::string as_string() const {
    return string_value;
  }
  int64_t as_int() const {
    return int_value;
  }
  double as_double() const {
    return double_value;
  }
  bool as_bool() const {
    return bool_value;
  }
  const std::map<std::string, ConfigValue>& as_object() const {
    return object_value;
  }
  const std::vector<ConfigValue>& as_array() const {
    return array_value;
  }

  std::optional<ConfigValue> get(const std::string& key) const {
    if (!is_object())
      return std::nullopt;
    auto it = object_value.find(key);
    if (it == object_value.end())
      return std::nullopt;
    return it->second;
  }
};

/**
 * Bar configuration
 */
struct BarConfig {
  enum class Position { Top, Bottom, Left, Right };

  Position position = Position::Top;
  uint32_t height = 30;
  std::string background = "#1e1e2e";
  std::string color = "#cdd6f4"; // Renamed from 'foreground' for consistency
  std::string font = "monospace";
  double size = 14.0;
};

/**
 * Widget configuration
 */
struct WidgetConfig {
  enum class Position { Left, Center, Right };

  std::string type;
  Position position = Position::Left;
  ConfigValue config;
};

/**
 * Top-level configuration
 */
struct Config {
  BarConfig bar;
  std::vector<WidgetConfig> widgets;
};

/**
 * ConfigManager - Loads and parses configuration files
 *
 * Supports JSON format for now (TOML could be added later)
 */
class ConfigManager {
public:
  ConfigManager() = default;
  ~ConfigManager() = default;

  /**
   * Load configuration from file
   * @param path Path to config file (JSON)
   * @return true on success
   */
  bool load(const std::string& path);

  /**
   * Load configuration from string
   * @param json JSON string
   * @return true on success
   */
  bool load_from_string(const std::string& json);

  /**
   * Get parsed configuration
   */
  const Config& get_config() const {
    return config_;
  }

  /**
   * Get last error message
   */
  const std::string& get_error() const {
    return error_;
  }

  /**
   * Get default config path
   */
  static std::string get_default_config_path();

  /**
   * Get config directory (where config file was loaded from)
   */
  const std::string& get_config_dir() const {
    return config_dir_;
  }

  /**
   * Resolve path relative to config directory
   * If path is absolute, returns it unchanged
   * If path starts with ~, expands home directory
   * Otherwise resolves relative to config file directory
   */
  std::string resolve_path(const std::string& path) const;

private:
  bool parse_json(const std::string& json);
  bool parse_bar_config(const ConfigValue& value);
  bool parse_widgets(const ConfigValue& value);

  BarConfig::Position parse_position(const std::string& pos);
  WidgetConfig::Position parse_widget_position(const std::string& pos);

  Config config_;
  std::string error_;
  std::string config_dir_; // Directory containing config file
};

} // namespace hyprbar
