#include "hyprbar/core/security_validator.h"
#include "hyprbar/core/logger.h"
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <unistd.h>

namespace hyprbar {

bool SecurityValidator::is_safe_command(const std::string& command) {
  if (command.empty()) {
    Logger::instance().warn("Empty command is not allowed");
    return false;
  }

  // Extract the first token (the actual command/program)
  size_t space_pos = command.find(' ');
  std::string program =
      space_pos != std::string::npos ? command.substr(0, space_pos) : command;

  // Must be absolute path or simple command name (no path separators)
  bool is_absolute = program[0] == '/';
  bool has_slash = program.find('/') != std::string::npos;

  if (!is_absolute && has_slash) {
    Logger::instance().warn(
        "Relative path commands are not allowed (security risk): {}", program);
    return false;
  }

  // If absolute path, check it exists and is executable
  if (is_absolute) {
    std::filesystem::path cmd_path(program);

    if (!std::filesystem::exists(cmd_path)) {
      Logger::instance().warn("Command does not exist: {}", program);
      return false;
    }

    if (!is_executable(cmd_path)) {
      Logger::instance().warn("Command is not executable: {}", program);
      return false;
    }

    // Check it's in a safe directory (not in /tmp, user writable dirs, etc.)
    std::string path_str = cmd_path.string();
    if (path_str.find("/tmp") == 0 || path_str.find("/var/tmp") == 0) {
      Logger::instance().warn(
          "Commands in /tmp or /var/tmp are not allowed (security risk): {}",
          program);
      return false;
    }
  }

  // Warn about dangerous metacharacters (but allow them for flexibility)
  // Users are responsible for their own configs
  if (contains_shell_metacharacters(command)) {
    Logger::instance().warn(
        "Command contains shell metacharacters (potential security risk): {}",
        command);
    // Don't block - allow but warn
  }

  return true;
}

bool SecurityValidator::is_within_allowed_dirs(
    const std::filesystem::path& path,
    const std::vector<std::filesystem::path>& allowed_dirs) {
  std::filesystem::path canonical_path;

  try {
    canonical_path = std::filesystem::canonical(path);
  } catch (const std::filesystem::filesystem_error& e) {
    Logger::instance().error("Failed to canonicalize path {}: {}",
                             path.string(), e.what());
    return false;
  }

  for (const auto& allowed : allowed_dirs) {
    std::filesystem::path canonical_allowed;
    try {
      canonical_allowed = std::filesystem::canonical(allowed);
    } catch (const std::filesystem::filesystem_error&) {
      continue; // Skip non-existent allowed dirs
    }

    // Check if path starts with allowed dir
    auto path_it = canonical_path.begin();
    auto allowed_it = canonical_allowed.begin();

    bool matches = true;
    while (allowed_it != canonical_allowed.end()) {
      if (path_it == canonical_path.end() || *path_it != *allowed_it) {
        matches = false;
        break;
      }
      ++path_it;
      ++allowed_it;
    }

    if (matches) {
      return true;
    }
  }

  return false;
}

std::filesystem::path
SecurityValidator::resolve_path_safely(const std::string& path,
                                       const std::filesystem::path& base_dir) {
  std::filesystem::path result;

  // Handle tilde expansion
  std::string expanded_path = path;
  if (!path.empty() && path[0] == '~') {
    const char* home = getenv("HOME");
    if (home) {
      expanded_path = std::string(home) + path.substr(1);
    }
  }

  // Make absolute
  if (std::filesystem::path(expanded_path).is_absolute()) {
    result = expanded_path;
  } else {
    result = base_dir / expanded_path;
  }

  // Canonicalize (resolves .. and symlinks)
  try {
    result = std::filesystem::canonical(result);
  } catch (const std::filesystem::filesystem_error& e) {
    throw std::runtime_error("Invalid path: " + path + " - " + e.what());
  }

  // Check not traversing outside base (for relative paths)
  if (!std::filesystem::path(path).is_absolute()) {
    std::filesystem::path canonical_base;
    try {
      canonical_base = std::filesystem::canonical(base_dir);
    } catch (const std::filesystem::filesystem_error& e) {
      throw std::runtime_error("Invalid base directory: " + base_dir.string() +
                               " - " + e.what());
    }

    // Verify result is within base
    auto result_it = result.begin();
    auto base_it = canonical_base.begin();

    while (base_it != canonical_base.end()) {
      if (result_it == result.end() || *result_it != *base_it) {
        throw std::runtime_error("Path traverses outside base directory: " +
                                 path);
      }
      ++result_it;
      ++base_it;
    }
  }

  return result;
}

bool SecurityValidator::contains_shell_metacharacters(const std::string& str) {
  // Common shell metacharacters that could be exploited
  const std::string dangerous_chars = ";|&$`<>(){}[]!*?";

  return std::any_of(str.begin(), str.end(), [&](char c) {
    return dangerous_chars.find(c) != std::string::npos;
  });
}

bool SecurityValidator::is_executable(const std::filesystem::path& path) {
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return false;
  }

  // Check if file is executable by user
  return (st.st_mode & S_IXUSR) != 0;
}

} // namespace hyprbar
