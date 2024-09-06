#include "injector.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <cwchar>
#include <exception>
#include <filesystem>

#include <iostream>
#include <string>

#include "common.h"
#include "process_info.h"

constexpr auto pipe_buffer_size = 4096;

std::atomic<HWND> atomic_hwnd{NULL};

HANDLE open_process_by_pid(uint32_t pid) {
  auto hr = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
  return hr;
}

[[noreturn]] void throw_system_error(const std::string& msg, DWORD dwErrCode = GetLastError()) {
	throw std::system_error(dwErrCode, std::system_category(), msg);
}

void hook_to_process(DWORD dwPID, std::wstring const &dllName) {
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, dwPID);

  bool bRes = true;

  if (hProcess == NULL) {
    DWORD dwErrCode = GetLastError();
    if (dwErrCode == ERROR_ACCESS_DENIED) {
      // TODO : Elevate injector if target is elevated
    } else {
      throw_system_error("Failed to open process handle");
    }

    return;
  }

  size_t nLen = wcslen(dllName.c_str()) * sizeof(WCHAR);
  HINSTANCE hKernel32 = GetModuleHandleW(L"KERNEL32.DLL");

  if (hKernel32 == NULL) {
    throw_system_error("Failed to get KERNEL32.DLL handle");
    return;
  }

  void *lpLoadLibraryW = reinterpret_cast<void*>(::GetProcAddress(hKernel32, "LoadLibraryW"));
  if (!lpLoadLibraryW) {
    throw_system_error("Failed to get LoadLibraryW address");
    return;
  }

  void *lpRemoteString =
      VirtualAllocEx(hProcess, nullptr, nLen + 1, MEM_COMMIT, PAGE_READWRITE);
  if (lpRemoteString == NULL) {
    throw_system_error("Failed to allocate in target");
    return;
  }

  bRes = WriteProcessMemory(hProcess, lpRemoteString, dllName.c_str(), nLen,
                            nullptr);
  if (!bRes) {
    throw_system_error("Could not write to process memory");
    return;
  }

  HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                      (LPTHREAD_START_ROUTINE)lpLoadLibraryW,
                                      lpRemoteString, 0, nullptr);

  if (hThread) {
    WaitForSingleObject(hThread, 500);
  }

  VirtualFreeEx(hProcess, lpRemoteString, 0, MEM_RELEASE);

  CloseHandle(hProcess);

  return;
}

int main(int argc, char *argv[]) {
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);

  // WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"テスト\n", 3, nullptr,
  // nullptr);

  std::filesystem::path dllPath =
      std::filesystem::current_path() / "libtarget.dll";
  dllPath = std::filesystem::absolute(dllPath);

  if (!std::filesystem::exists(dllPath)) {
    std::cerr << "Target DLL not found" << std::endl;
    return 1;
  }

  HANDLE pipe_handle =
      CreateNamedPipeW(common::hookdict_pipe_name, PIPE_ACCESS_INBOUND,
                       PIPE_TYPE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1,
                       pipe_buffer_size, pipe_buffer_size, 500, nullptr);

  std::cout << "Created pipe" << std::endl;

  try {
    std::string proc_name;

    if (argc > 1 && common::is_valid_executable_name(argv[1])) {
      proc_name = std::string(argv[1]);
    } else {
      std::cout << "Process name: ";
      std::cin >> proc_name;
    }

    if (proc_name.empty()) {
      std::cerr << "Empty process name!" << std::endl;
      return 1;
    }

    std::transform(proc_name.begin(), proc_name.end(), proc_name.begin(),
                   [](char c) { return static_cast<char>(std::toupper(c)); });

    std::vector procs = process_info::enum_processes();

    auto it = std::find_if(
        procs.begin(), procs.end(), [&](const process_info &proc_info) {
          auto filename = proc_info.m_module_name.filename().string();
          std::transform(
              filename.begin(), filename.end(), filename.begin(),
              [](char c) { return static_cast<char>(std::toupper(c)); });

          return !filename.empty() && filename == proc_name;
        });

    if (it != procs.end()) {
      hook_to_process(it->m_pid, dllPath);
    } else {
      std::cerr << "Failed to find process with name: " << proc_name
                << std::endl;
      return 1;
    }

  } catch (std::exception const &ex) {
    std::cerr << ex.what();
    return 1;
  }

  // auto thread = std::thread(thread_func);

  // auto shmem_file = create_shmem_file();
  // auto shmem_view = create_mapview(shmem_file);

  bool bRes = ConnectNamedPipe(pipe_handle, nullptr);

  if (!bRes) {
    DWORD dwErrCode = GetLastError();
    if (dwErrCode != ERROR_PIPE_CONNECTED) {
      CloseHandle(pipe_handle);
      return 1;
    }
  }

  std::array<char, pipe_buffer_size> buf{};
  while (true) {
    using buf_element = decltype(buf)::value_type;
    buf.fill('\0');

    DWORD nBytesRead = 0;
    bRes = ::ReadFile(pipe_handle, buf.data(),
                      (buf.size() - 1) * sizeof(buf_element), &nBytesRead,
                      nullptr);

    if (!bRes) {
      CloseHandle(pipe_handle);
      return 1;
    }

    std::string_view sv(buf.data(), nBytesRead);

    // std::u16string u16_msg(256, '\0');
    // int32_t len =
    //     ucnv_toUChars(conv, u16_msg.data(), u16_msg.size() *
    //     sizeof(char16_t),
    //                   buf.data(), -1, &uec);

    // if (U_FAILURE(uec)) {
    //   std::cout << "ICU error" << std::endl;
    //   return 1;
    // }

    // u16_msg.resize(len);
    // common::write_stdout_console(std::move(u16_msg));
  }

  return 0;
}
