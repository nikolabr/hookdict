#include "injector.h"

#include <cstddef>
#include <cstdint>
#include <cwchar>

#include <rpc/server.h>
#include <string>
#include <system_error>

#include <boost/outcome.hpp>
#include <boost/outcome/outcome.hpp>

#include "boost/winapi/character_code_conversion.hpp"
#include "common.h"
#include "process_info.h"

namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;

HANDLE open_process_by_pid(uint32_t pid) {
  auto hr = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
  return hr;
}

outcome::result<void> hook_to_process(DWORD dwPID,
                                      std::wstring const &dllName) {
  HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, dwPID);

  bool bRes = true;

  if (hProcess == NULL) {
    DWORD dwErrCode = GetLastError();
    if (dwErrCode == ERROR_ACCESS_DENIED) {
      // TODO : Elevate injector if target is elevated
    } else {
      return outcome::failure(
          std::error_code(dwErrCode, std::system_category()));
    }
  }

  size_t nLen = wcslen(dllName.c_str()) * sizeof(WCHAR);
  HINSTANCE hKernel32 = GetModuleHandleW(L"KERNEL32.DLL");

  if (hKernel32 == NULL) {
    return outcome::failure(
        std::error_code(::GetLastError(), std::system_category()));
  }

  void *lpLoadLibraryW =
      reinterpret_cast<void *>(::GetProcAddress(hKernel32, "LoadLibraryW"));
  if (!lpLoadLibraryW) {
    return outcome::failure(
        std::error_code(::GetLastError(), std::system_category()));
  }

  void *lpRemoteString =
      VirtualAllocEx(hProcess, nullptr, nLen + 1, MEM_COMMIT, PAGE_READWRITE);
  if (lpRemoteString == NULL) {
    return outcome::failure(
        std::error_code(::GetLastError(), std::system_category()));
  }

  bRes = WriteProcessMemory(hProcess, lpRemoteString, dllName.c_str(), nLen,
                            nullptr);
  if (!bRes) {
    return outcome::failure(
        std::error_code(::GetLastError(), std::system_category()));
  }

  HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
                                      (LPTHREAD_START_ROUTINE)lpLoadLibraryW,
                                      lpRemoteString, 0, nullptr);

  if (hThread) {
    WaitForSingleObject(hThread, 500);
  }
  
  VirtualFreeEx(hProcess, lpRemoteString, 0, MEM_RELEASE);

  CloseHandle(hProcess);

  return outcome::success();
}

void on_target_msg(std::string const& text, uint32_t codepage) {
  if (codepage != 932) {
    std::cerr << "Target codepage is not Shift-JIS" << std::endl;
  }
  std::size_t len = ::MultiByteToWideChar(codepage, 0, text.c_str(), text.size(), nullptr, 0);

  std::wstring out(len, L'\0');
  DWORD res = ::MultiByteToWideChar(codepage, 0, text.c_str(), text.size(), out.data(), out.size());
  if (res == 0) {
    std::cerr << "Multibyte conversion error" << std::endl;
  }
  else {
    common::write_stdout_console(out);
  }
}

int main(int argc, char *argv[]) {
  // Set UTF-16 console
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);

  rpc::server server("127.0.0.1", common::hookdict_port);
  
  server.bind("target_msg", on_target_msg);

  server.async_run();
  
  std::filesystem::path dllPath =
      std::filesystem::current_path() / "libtarget.dll";
  dllPath = std::filesystem::absolute(dllPath);

  if (!std::filesystem::exists(dllPath)) {
    std::cerr << "Target DLL does not exist";
    return 1;
  }

  std::string proc_name;
  if (argc > 1 && common::is_valid_executable_name(argv[1])) {
    proc_name = std::string(argv[1]);
  }

  if (proc_name.empty()) {
    std::cerr << "Executable name invalid";
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
    auto result = hook_to_process(it->m_pid, dllPath);
    if (!result) {
      std::cerr << "Error in hooking process: " << result.as_failure().error() << std::endl;
    }
  } else {
    std::cerr << "Could not find process:" << proc_name << std::endl;
    return 1;
  }

  while (true) {};

  return 0;
}
