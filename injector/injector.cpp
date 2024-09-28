#include "injector.h"

#include "common.h"
#include "msg.h"
#include "process_info.h"
#include "server.h"

#include <cstdint>

#include <filesystem>
#include <processenv.h>
#include <string>
#include <system_error>

#include <boost/log/trivial.hpp>

#include <boost/outcome.hpp>
#include <boost/outcome/outcome.hpp>

#include <boost/filesystem.hpp>

#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/interprocess_fwd.hpp>
#include <windows.h>

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

  HANDLE hThread = ::CreateRemoteThread(hProcess, nullptr, 0,
                                      (LPTHREAD_START_ROUTINE)lpLoadLibraryW,
                                      lpRemoteString, 0, nullptr);

  if (hThread) {
    WaitForSingleObject(hThread, 500);
  }

  VirtualFreeEx(hProcess, lpRemoteString, 0, MEM_RELEASE);

  CloseHandle(hProcess);

  return outcome::success();
}

boost::interprocess::mapped_region create_shm()
{
  using namespace boost::interprocess;
  
  windows_shared_memory shm(create_only, common::shared_memory_name, read_write, sizeof(common::shared_memory));
  return mapped_region(shm, read_write);
}

int main(int argc, char *argv[]) {
  // Set UTF-16 console
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);

  std::filesystem::path dllPath =
    std::filesystem::weakly_canonical(std::filesystem::path(argv[0]))
    .parent_path() /
      "../target/libhookdict_target.dll";

  if (!std::filesystem::exists(dllPath)) {
    BOOST_LOG_TRIVIAL(fatal) << "Target DLL does not exist";
    return 1;
  }

  std::string proc_name;
  if (argc > 1 && common::is_valid_executable_name(argv[1])) {
    proc_name = std::string(argv[1]);
  }

  if (proc_name.empty()) {
    BOOST_LOG_TRIVIAL(fatal) << "Executable name invalid";
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
      std::cerr << "Error in hooking process: " << result.as_failure().error()
                << std::endl;
    }

  } else {
    // std::cerr << "Could not find process:" << proc_name << std::endl;
    BOOST_LOG_TRIVIAL(fatal) << "Could not find process: " << proc_name;
    return 1;
  }
  
  auto region = create_shm();

  common::shared_memory* shm_ptr = new (region.get_address()) common::shared_memory;
  std::fill(shm_ptr->m_img_buf.m_buf.begin(), shm_ptr->m_img_buf.m_buf.end(), 0);
  
  auto r = run_server(shm_ptr, {});
  if (!r.has_value()) {
    BOOST_LOG_TRIVIAL(error) << r.as_failure().error();
  }

  return 0;
}
