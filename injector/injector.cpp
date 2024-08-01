﻿#include "injector.h"

#include <array>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "common.h"
#include "process_info.h"

#include <wil/resource.h>
#include <wil/result_macros.h>

constexpr auto nPipeBufferSize = 4096;

wil::unique_process_handle open_process_by_pid(uint32_t pid) {
  auto hr = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
  LOG_LAST_ERROR_IF_NULL(hr);

  wil::unique_process_handle process_handle(hr);
  return process_handle;
}

void hook_to_process(DWORD dwPID, std::wstring const &dllName) {
  auto process_handle = open_process_by_pid(dwPID);
  THROW_LAST_ERROR_IF_NULL(process_handle);

  wil::unique_hmodule kernel32(::GetModuleHandleW(L"KERNEL32.DLL"));
  THROW_LAST_ERROR_IF_NULL(kernel32);

  FARPROC load_library_w = GetProcAddress(kernel32.get(), "LoadLibraryW");
  THROW_LAST_ERROR_IF_NULL(load_library_w);

  size_t string_len = wcslen(dllName.c_str()) * sizeof(wchar_t);
  wil::unique_virtualalloc_ptr<void> remote_string(
      ::VirtualAllocEx(process_handle.get(), nullptr, string_len + 1,
                       MEM_COMMIT, PAGE_READWRITE));
  THROW_LAST_ERROR_IF_NULL(remote_string);

  THROW_IF_WIN32_BOOL_FALSE(::WriteProcessMemory(
      process_handle.get(), remote_string.get(),
      reinterpret_cast<void const *>(dllName.c_str()), string_len, nullptr));

  wil::unique_handle thread_handle(::CreateRemoteThread(
      process_handle.get(), nullptr, 0, (LPTHREAD_START_ROUTINE)load_library_w,
      remote_string.get(), 0, nullptr));

  if (thread_handle) {
    wil::handle_wait(thread_handle.get(), 500);
  }

  VirtualFreeEx(process_handle.get(), remote_string.release(), 0, MEM_RELEASE);

  return;
}

int main(int argc, char *argv[]) {
  std::filesystem::path dllPath =
      std::filesystem::current_path() / "../target/hookdict_target.dll";
  dllPath = std::filesystem::absolute(dllPath);

  if (!std::filesystem::exists(dllPath)) {
    std::wcerr << "Target DLL not found" << std::endl;
    return 1;
  }

  wil::unique_hfile pipe_handle(CreateNamedPipeW(
      common::hookdict_pipe_name, PIPE_ACCESS_INBOUND,
      PIPE_TYPE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 0, 0, 0, nullptr));

  THROW_LAST_ERROR_IF_MSG(!pipe_handle, "Failed to open pipe!");

  std::wcout << "Created pipe" << std::endl;

  try {
    std::wstring proc_name;
    std::wcout << "Process name: ";
    std::wcin >> proc_name;

    std::transform(proc_name.begin(), proc_name.end(), proc_name.begin(),
                   towupper);

    std::vector procs = process_info::enum_processes();

    auto it = std::find_if(
        procs.begin(), procs.end(), [&](const process_info &proc_info) {
          auto filename = proc_info.m_module_name.filename().wstring();
          std::transform(filename.begin(), filename.end(), filename.begin(),
                         towupper);

          return !filename.empty() && filename == proc_name;
        });

    if (it != procs.end()) {
      hook_to_process(it->m_pid, dllPath);
    } else {
      std::wcerr << "Failed to find process with name: " << proc_name << std::endl;
      return 1;
    }

  } catch (std::exception const &ex) {
    std::wcerr << ex.what();
    return 1;
  }

  while (true) {
    bool bRes = ConnectNamedPipe(pipe_handle.get(), nullptr);

    if (!bRes) {
      DWORD dwErrCode = GetLastError();
      if (dwErrCode != ERROR_PIPE_CONNECTED) {
        CloseHandle(pipe_handle.get());
        return 1;
      }
    }

    std::array<wchar_t, nPipeBufferSize> buf{};
    using buf_element = decltype(buf)::value_type;
    buf.fill('\0');

    DWORD nBytesRead = 0;
    bRes = ReadFile(pipe_handle.get(), buf.data(), (buf.size() - 1) * sizeof(buf_element),
                    &nBytesRead, nullptr);

    if (!bRes) {
      CloseHandle(pipe_handle.get());
      return 1;
    }

    buf[nBytesRead / sizeof(buf_element)] = '\0';
    std::wcout << buf.data() << std::endl;
  }

  return 0;
}
