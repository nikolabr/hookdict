#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <windows.h>

#include "common.h"

struct process_info {
  uint32_t m_pid;
  std::filesystem::path m_module_name;

  process_info(uint32_t pid, auto &&basename)
      : m_pid(pid), m_module_name(basename) {}

  process_info(uint32_t pid)
      : m_pid(pid), m_module_name(get_process_name(pid)) {}

  /*
    Returns empty string if unable to open process
   */
  static std::wstring get_process_name(uint32_t pid);
  static std::vector<process_info> enum_processes();

  static uint32_t get_current_pid() { return ::GetCurrentProcessId(); }
};
