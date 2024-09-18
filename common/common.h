// common.h : Header file for your target.

#pragma once

#include <sdkddkver.h>
#include <string>
#include <winapifamily.h>

#include <windows.h>
#include <psapi.h>

#include <array>
#include <filesystem>

namespace common {
constexpr static uint16_t hookdict_port = 18090;

static std::filesystem::path get_module_file_name_w(HMODULE handle) {
  std::array<wchar_t, MAX_PATH> out{};
  
  DWORD res = ::GetModuleFileNameW(handle, out.data(), out.size());
  return std::filesystem::path(out.begin(), out.begin() + res);
}

static std::filesystem::path get_module_file_name_w() {
  return get_module_file_name_w(NULL);
}

template <typename H>
static std::filesystem::path get_module_file_name_w(H &&handle) {
  return get_module_file_name_w(handle.get());
}

static bool is_valid_executable_name(auto &&name) {
  std::filesystem::path p(name);
  if (!p.has_extension()) {
    return false;
  }

  auto ext = std::move(p).extension();

  return ext == ".EXE" || ext == ".exe";
}

template <typename T,
          std::enable_if_t<std::is_constructible<std::wstring, T>::value,
                           bool> = true>
std::size_t write_stdout_console(T &&msg) {
  HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  const wchar_t *buf = msg.c_str();
  DWORD len = msg.size();
  DWORD out = 0;

  ::WriteConsoleW(stdout_handle, reinterpret_cast<const void *>(buf), len, &out,
                  nullptr);

  return out;
}

template <
    typename T,
    std::enable_if_t<std::is_constructible<std::string, T>::value, bool> = true>
std::size_t write_stdout_console(T &&msg) {
  HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  const wchar_t *buf = msg.c_str();
  DWORD len = msg.size();
  DWORD out = 0;

  ::WriteConsoleA(stdout_handle, reinterpret_cast<const void *>(buf), len, &out,
                  nullptr);

  return out;
}
} // namespace common
