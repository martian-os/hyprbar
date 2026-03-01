#pragma once

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace hyprbar {

/**
 * Logger - Simple logging system with severity levels
 *
 * Thread-safe singleton logger with configurable output and formatting.
 */
class Logger {
public:
  enum class Level { Debug = 0, Info = 1, Warn = 2, Error = 3 };

  static Logger &instance();

  // Non-copyable, non-movable
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  /**
   * Set minimum log level (messages below this are ignored)
   */
  void set_level(Level level);

  /**
   * Get current minimum log level
   */
  Level get_level() const { return min_level_; }

  /**
   * Log a message with specified level
   */
  void log(Level level, const std::string &message);

  /**
   * Convenience methods for specific levels
   */
  void debug(const std::string &message) { log(Level::Debug, message); }
  void info(const std::string &message) { log(Level::Info, message); }
  void warn(const std::string &message) { log(Level::Warn, message); }
  void error(const std::string &message) { log(Level::Error, message); }

  /**
   * Template methods for formatted logging
   */
  template <typename... Args>
  void debug(const std::string &format, Args &&...args) {
    log(Level::Debug, format_string(format, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void info(const std::string &format, Args &&...args) {
    log(Level::Info, format_string(format, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void warn(const std::string &format, Args &&...args) {
    log(Level::Warn, format_string(format, std::forward<Args>(args)...));
  }

  template <typename... Args>
  void error(const std::string &format, Args &&...args) {
    log(Level::Error, format_string(format, std::forward<Args>(args)...));
  }

  /**
   * Set output stream (default: stderr)
   */
  void set_output(std::ostream *output) { output_ = output; }

  /**
   * Enable/disable colored output (ANSI codes)
   */
  void set_colored(bool colored) { colored_ = colored; }

private:
  Logger();
  ~Logger() = default;

  std::string level_string(Level level) const;
  std::string level_color(Level level) const;
  std::string timestamp() const;

  template <typename T> std::string to_string(T &&value) {
    std::ostringstream oss;
    oss << std::forward<T>(value);
    return oss.str();
  }

  template <typename... Args>
  std::string format_string(const std::string &format, Args &&...args) {
    std::string result = format;
    std::string values[] = {to_string(std::forward<Args>(args))...};
    size_t index = 0;

    size_t pos = 0;
    while ((pos = result.find("{}", pos)) != std::string::npos) {
      if (index < sizeof...(Args)) {
        result.replace(pos, 2, values[index++]);
        pos += values[index - 1].length();
      } else {
        break;
      }
    }
    return result;
  }

  Level min_level_;
  std::ostream *output_;
  bool colored_;
};

// Convenience macros
#define LOG_DEBUG(msg) hyprbar::Logger::instance().debug(msg)
#define LOG_INFO(msg) hyprbar::Logger::instance().info(msg)
#define LOG_WARN(msg) hyprbar::Logger::instance().warn(msg)
#define LOG_ERROR(msg) hyprbar::Logger::instance().error(msg)

} // namespace hyprbar
