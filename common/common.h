﻿// common.h : Header file for your target.

#pragma once

#include <Windows.h>

#include <intrin.h>
#include <windef.h>
#include <winnt.h>

#include <winbase.h>

#include <stringapiset.h>
#include <winuser.h>
#include <wingdi.h>
#include <processthreadsapi.h>
#include <wincon.h>

#include <array>
#include <filesystem>


namespace common {
constexpr static wchar_t hookdict_pipe_name[] = L"\\\\.\\pipe\\hookdict_pipe";

constexpr static wchar_t hookdict_shmem[] = L"Global\\HookdictSharedBuffer";
constexpr std::size_t shmem_buffer_size = 0x1000000;

constexpr static wchar_t hookdict_event[] = L"HookdictSharedEvent";
constexpr static wchar_t hookdict_semaphore[] = L"HookdictSemaphore";

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

template <
    typename T,
    std::enable_if_t<std::constructible_from<std::u16string, T>, bool> = true>
std::size_t write_stdout_console(T &&msg) {
  HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  const char16_t *buf = msg.c_str();
  DWORD len = msg.size();
  DWORD out = 0;

  WriteConsoleW(
      stdout_handle, reinterpret_cast<const void *>(buf), len, &out, nullptr);

  return out;
}

template <typename T, std::enable_if_t<std::constructible_from<std::string, T>,
                                       bool> = true>
std::size_t write_stdout_console(T &&msg) {
  HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

  const wchar_t *buf = msg.c_str();
  DWORD len = msg.size();
  DWORD out = 0;

  WriteConsoleA(
      stdout_handle, reinterpret_cast<const void *>(buf), len, &out, nullptr);

  return out;
}
} // namespace common
