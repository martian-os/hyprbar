#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace hyprbar {

/**
 * Security utilities for validating user input
 */
class SecurityValidator {
public:
  /**
   * Validate script command for safety
   * @param command Command to validate
   * @return true if command appears safe
   */
  static bool is_safe_command(const std::string& command);

  /**
   * Check if path is within allowed directories
   * @param path Path to check
   * @param allowed_dirs List of allowed base directories
   * @return true if path is within allowed dirs
   */
  static bool is_within_allowed_dirs(
      const std::filesystem::path& path,
      const std::vector<std::filesystem::path>& allowed_dirs);

  /**
   * Resolve and canonicalize path safely
   * @param path Path to resolve
   * @param base_dir Base directory for relative paths
   * @return Canonical absolute path
   * @throws std::runtime_error if path is invalid or traverses outside base
   */
  static std::filesystem::path
  resolve_path_safely(const std::string& path,
                      const std::filesystem::path& base_dir);

private:
  // Check if string contains shell metacharacters
  static bool contains_shell_metacharacters(const std::string& str);

  // Check if path exists and is executable
  static bool is_executable(const std::filesystem::path& path);
};

} // namespace hyprbar
