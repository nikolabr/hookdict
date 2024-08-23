#include "injector.h"

#include <array>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#include "common.h"
#include "process_info.h"

#include <unicode/ucnv_err.h>
#include <unicode/urename.h>
#include <unicode/utypes.h>
#include <wil/resource.h>
#include <wil/result_macros.h>

constexpr auto pipe_buffer_size = 4096;

wil::unique_event hwnd_event(wil::EventOptions::ManualReset);
std::atomic<HWND> atomic_hwnd{NULL};

void thread_func() {
  hwnd_event.wait();

  HWND hwnd = atomic_hwnd;
  wil::unique_hdc wnd_dc(::GetDC(hwnd));
  LOG_LAST_ERROR_IF_NULL(wnd_dc);

  std::cout << "Opened window DC" << std::endl;

  wil::unique_hdc mem_dc(::CreateCompatibleDC(wnd_dc.get()));
  LOG_LAST_ERROR_IF_NULL(mem_dc);

  RECT rcWnd;
  GetClientRect(hwnd, &rcWnd);

  wil::unique_hbitmap hbm(::CreateCompatibleBitmap(
      wnd_dc.get(), rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top));
  LOG_LAST_ERROR_IF_NULL(hbm);

  BITMAP bmp;

  ::SelectObject(mem_dc.get(), hbm.get());

  WINBOOL b = BitBlt(mem_dc.get(), 0, 0, rcWnd.right - rcWnd.left,
                     rcWnd.bottom - rcWnd.top, wnd_dc.get(), 0, 0, SRCCOPY);
  LOG_LAST_ERROR_IF(b == FALSE);

  if (!b) {
    return;
  }

  GetObject(hbm.get(), sizeof(BITMAP), &bmp);
  std::cout << "BMP width: " << bmp.bmWidth << " BMP height: " << bmp.bmHeight
            << std::endl;

  BITMAPFILEHEADER bmfHeader;
  BITMAPINFOHEADER bi;

  bi.biSize = sizeof(BITMAPINFOHEADER);
  bi.biWidth = bmp.bmWidth;
  bi.biHeight = bmp.bmHeight;
  bi.biPlanes = 1;
  bi.biBitCount = 32;
  bi.biCompression = BI_RGB;
  bi.biSizeImage = 0;
  bi.biXPelsPerMeter = 0;
  bi.biYPelsPerMeter = 0;
  bi.biClrUsed = 0;
  bi.biClrImportant = 0;

  DWORD dwBmpSize =
      ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;

  std::cout << "BMP size: " << dwBmpSize << std::endl;

  std::unique_ptr<char[]> dib = std::make_unique<char[]>(dwBmpSize);
  DWORD res = ::GetDIBits(wnd_dc.get(), hbm.get(), 0, bmp.bmHeight, dib.get(),
                          reinterpret_cast<BITMAPINFO *>(&bi), DIB_RGB_COLORS);

  LOG_LAST_ERROR_IF(res == 0);

  {
    std::ofstream out("output.bmp", std::ios::binary | std::ios::trunc);
    DWORD dwSizeofDIB =
        dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmfHeader.bfOffBits =
        (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42;

    out.write(reinterpret_cast<char *>(&bmfHeader), sizeof(BITMAPFILEHEADER));
    out.write(reinterpret_cast<char *>(&bi), sizeof(BITMAPINFOHEADER));
    out.write(dib.get(), dwBmpSize);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  std::cout << "Copy loop stopping" << std::endl;
}

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

wil::unique_handle create_shmem_file() {
  auto res = wil::unique_handle(
      CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                         common::shmem_buffer_size, common::hookdict_shmem));
  THROW_LAST_ERROR_IF_NULL(res);
  return res;
}

wil::unique_mapview_ptr<> create_mapview(wil::unique_handle const &file) {
  auto res = wil::unique_mapview_ptr<>(MapViewOfFile(
      file.get(), FILE_MAP_ALL_ACCESS, 0, 0, common::shmem_buffer_size));
  THROW_LAST_ERROR_IF_NULL(res);
  return res;
}

int main(int argc, char *argv[]) {
  SetConsoleOutputCP(65001);
  SetConsoleCP(65001);

  // WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), L"テスト\n", 3, nullptr,
  // nullptr);

  std::filesystem::path dllPath =
      std::filesystem::current_path() / "../target/hookdict_target.dll";
  dllPath = std::filesystem::absolute(dllPath);

  if (!std::filesystem::exists(dllPath)) {
    std::cerr << "Target DLL not found" << std::endl;
    return 1;
  }

  wil::unique_hfile pipe_handle(
      CreateNamedPipeW(common::hookdict_pipe_name, PIPE_ACCESS_INBOUND,
                       PIPE_TYPE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1,
                       pipe_buffer_size, pipe_buffer_size, 500, nullptr));

  THROW_LAST_ERROR_IF_MSG(!pipe_handle, "Failed to open pipe!");

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
                   towupper);

    std::vector procs = process_info::enum_processes();

    auto it = std::find_if(
        procs.begin(), procs.end(), [&](const process_info &proc_info) {
          auto filename = proc_info.m_module_name.filename().string();
          std::transform(filename.begin(), filename.end(), filename.begin(),
                         towupper);

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

  bool bRes = ConnectNamedPipe(pipe_handle.get(), nullptr);

  if (!bRes) {
    DWORD dwErrCode = GetLastError();
    if (dwErrCode != ERROR_PIPE_CONNECTED) {
      CloseHandle(pipe_handle.get());
      return 1;
    }
  }

  UErrorCode uec = U_ZERO_ERROR;
  UConverter *conv = ucnv_open("shift_jis", &uec);

  std::array<char, pipe_buffer_size> buf{};
  while (true) {
    using buf_element = decltype(buf)::value_type;
    buf.fill('\0');

    DWORD nBytesRead = 0;
    bRes = ::ReadFile(pipe_handle.get(), buf.data(),
                      (buf.size() - 1) * sizeof(buf_element), &nBytesRead,
                      nullptr);

    if (!bRes) {
      CloseHandle(pipe_handle.get());
      return 1;
    }

    std::string_view sv(buf.data(), nBytesRead);
    if (sv.find("WINDOWHANDLE") == 0) {
      std::cout << sv << std::endl;

      sv.remove_prefix(12);

      uint32_t hint = std::stoul(std::string(sv));
      static_assert(sizeof(uint32_t) == sizeof(HWND));

      HWND hwnd = reinterpret_cast<HWND>(hint);
      atomic_hwnd = hwnd;
      hwnd_event.SetEvent();

      continue;
    }

    std::u16string u16_msg(256, '\0');
    int32_t len =
        ucnv_toUChars(conv, u16_msg.data(), u16_msg.size() * sizeof(char16_t),
                      buf.data(), -1, &uec);

    if (U_FAILURE(uec)) {
      std::cout << "ICU error" << std::endl;
      return 1;
    }

    u16_msg.resize(len);
    common::write_stdout_console(std::move(u16_msg));
  }

  return 0;
}
