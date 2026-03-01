#include "hyprbar/core/logger.h"
#include <ctime>
#include <mutex>

namespace hyprbar {

static std::mutex log_mutex;

Logger::Logger()
    : min_level_(Level::Info), output_(&std::cerr), colored_(true) {
}

Logger& Logger::instance() {
  static Logger instance;
  return instance;
}

void Logger::set_level(Level level) {
  min_level_ = level;
}

std::string Logger::level_string(Level level) const {
  switch (level) {
  case Level::Debug:
    return "DEBUG";
  case Level::Info:
    return "INFO ";
  case Level::Warn:
    return "WARN ";
  case Level::Error:
    return "ERROR";
  default:
    return "?????";
  }
}

std::string Logger::level_color(Level level) const {
  if (!colored_) {
    return "";
  }

  switch (level) {
  case Level::Debug:
    return "\033[36m"; // Cyan
  case Level::Info:
    return "\033[32m"; // Green
  case Level::Warn:
    return "\033[33m"; // Yellow
  case Level::Error:
    return "\033[31m"; // Red
  default:
    return "";
  }
}

std::string Logger::timestamp() const {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()) %
            1000;

  std::tm tm_buf;
  localtime_r(&time, &tm_buf);

  std::ostringstream oss;
  oss << std::put_time(&tm_buf, "%H:%M:%S") << '.' << std::setfill('0')
      << std::setw(3) << ms.count();
  return oss.str();
}

void Logger::log(Level level, const std::string& message) {
  if (level < min_level_) {
    return;
  }

  std::lock_guard<std::mutex> lock(log_mutex);

  std::string color = level_color(level);
  std::string reset = colored_ ? "\033[0m" : "";

  *output_ << color << "[" << timestamp() << "] " << "["
      << level_string(level) << "] " << message << reset << std::endl;
}

} // namespace hyprbar
