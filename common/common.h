// common.h : Header file for your target.

#pragma once

/*
  WIL options

  https://github.com/microsoft/wil/pull/454
 */
#include <concepts>
#include <string>
#include <type_traits>
#define _Frees_ptr_
#define _Frees_ptr_opt_
#define _Post_z_
#define _Pre_maybenull_
#define _Pre_opt_valid_
#define _Pre_valid_
#define _Ret_opt_bytecap_(size)
#define _Translates_last_error_to_HRESULT_
#define _Translates_NTSTATUS_to_HRESULT_(status)
#define _Translates_Win32_to_HRESULT_(err)
#define InterlockedDecrementNoFence InterlockedDecrement
#define InterlockedIncrementNoFence InterlockedIncrement
#define WIL_NO_SLIM_EVENT

#include <windows.h>
#include <winsock2.h>

#include <array>
#include <filesystem>
#include <memory>

#include <wil/stl.h>

#define _MEMORY_
#include <wil/resource.h>

#include <wil/result_macros.h>

namespace common {
constexpr static wchar_t hookdict_pipe_name[] = L"\\\\.\\pipe\\hookdict_pipe";

static std::filesystem::path get_module_file_name_w(HMODULE handle) {
  std::array<wchar_t, MAX_PATH> out{};

  DWORD res = ::GetModuleFileNameW(handle, out.data(), out.size());

  THROW_LAST_ERROR_IF(res == 0);

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
          std::enable_if_t<std::constructible_from<std::u16string, T>, bool> = true>
std::size_t write_stdout_console(T &&msg) {
  HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  THROW_LAST_ERROR_IF_NULL(stdout_handle);

  const char16_t *buf = msg.c_str();
  DWORD len = msg.size();
  DWORD out = 0;

  THROW_IF_WIN32_BOOL_FALSE(WriteConsoleW(
      stdout_handle, reinterpret_cast<const void *>(buf), len, &out, nullptr));

  return out;
}

template <typename T,
          std::enable_if_t<std::constructible_from<std::string, T>, bool> = true>
std::size_t write_stdout_console(T &&msg) {
  HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  THROW_LAST_ERROR_IF_NULL(stdout_handle);

  const wchar_t *buf = msg.c_str();
  DWORD len = msg.size();
  DWORD out = 0;

  THROW_IF_WIN32_BOOL_FALSE(WriteConsoleA(
      stdout_handle, reinterpret_cast<const void *>(buf), len, &out, nullptr));

  return out;
}
} // namespace common
