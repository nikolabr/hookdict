#include "server.h"
#include "common.h"
#include "msg.h"

#include <fileapi.h>
#include <libloaderapi.h>
#include <thread>

#include <boost/log/trivial.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/lockfree/queue.hpp>
#include <boost/winapi/character_code_conversion.hpp>

#include <gdiplus.h>
#include <windows.h>

static boost::lockfree::queue<wchar_t> text_queue(4096);
static std::atomic<HWND> target_hwnd = NULL;

outcome::result<std::wstring> on_target_msg(auto const &text,
                                            uint32_t codepage) {
  if (codepage != 932) {
    std::cerr << "Target codepage is not Shift-JIS" << std::endl;
  }
  std::size_t len =
      ::MultiByteToWideChar(codepage, 0, text.c_str(), text.size(), nullptr, 0);

  std::wstring out(len, L'\0');
  DWORD res = ::MultiByteToWideChar(codepage, 0, text.c_str(), text.size(),
                                    out.data(), out.size());

  return out;
}

struct message_handler {
  auto operator()(common::target_connected_message_t const &msg) const {
    BOOST_LOG_TRIVIAL(info) << "Target connected";
    BOOST_LOG_TRIVIAL(info) << "Window name: " << msg.m_window_name;
    HWND hwnd = ::FindWindowA(nullptr, msg.m_window_name.c_str());
    if (hwnd == NULL) {
      BOOST_LOG_TRIVIAL(warning) << "Target window not found";
    }
    target_hwnd = hwnd;
  };

  auto operator()(common::target_generic_message_t const &msg) const {
    BOOST_LOG_TRIVIAL(debug) << "Received generic message";
    auto wstr = on_target_msg(msg.m_text, msg.m_cp);
    if (wstr.has_value()) {
      for (const wchar_t c : wstr.value()) {
        text_queue.push(c);
      }
    }
  };

  auto operator()(common::target_close_message_t const &) const {
    BOOST_LOG_TRIVIAL(info) << "Target closed";
    throw std::runtime_error("Target closed");
  }
};

static outcome::result<void> log_file_writer_thread(std::stop_token st) {
  HANDLE log_file = ::CreateFile("target.log", GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                 CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (log_file == INVALID_HANDLE_VALUE) {
    BOOST_LOG_TRIVIAL(warning) << "Failed to open log file";
  }

  while (!st.stop_requested()) {
    // Memory mapped files might be a better solution
    std::size_t consumed = text_queue.consume_all([&](const wchar_t c) {
      ::WriteFile(log_file, &c, sizeof(wchar_t), nullptr, nullptr);
    });

    if (consumed > 0) {
      ::FlushFileBuffers(log_file);
    }
  }

  ::CloseHandle(log_file);
  return outcome::success();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  
  case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);

      // FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
    
      if (target_hwnd != NULL) {
	// ::PrintWindow(target_hwnd, ps.hdc, PW_CLIENTONLY);
	// ::SendMessage(target_hwnd, WM_PRINTCLIENT, (DWORD)ps.hdc, PRF_OWNED | PRF_CLIENT | PRF_NONCLIENT);
	
	RECT rect;
	::GetClientRect(target_hwnd, &rect);
	BOOST_LOG_TRIVIAL(debug) << rect.right - rect.left << " " << rect.bottom - rect.top;
	HDC target_dc = GetDC(target_hwnd);
	assert(target_dc != NULL);

	DWORD dwRes = ::BitBlt(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top, target_dc, 0, 0, SRCCOPY);
	
	BOOST_LOG_TRIVIAL(debug) << "Printing window " << dwRes;

	ReleaseDC(NULL, target_dc);
      }

      EndPaint(hwnd, &ps);
      return 0;
    }
  default:
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
  }
}

DWORD wnd_thread(std::stop_token st) {
  const char CLASS_NAME[] = "Sample Window Class";

  WNDCLASS wc = {};

  HINSTANCE hInstance = GetModuleHandle(0);

  wc.lpfnWndProc = WindowProc;
  wc.hInstance = GetModuleHandle(0);
  wc.lpszClassName = CLASS_NAME;

  RegisterClass(&wc);

  HWND hwnd =
      CreateWindowEx(0,                          // Optional window styles.
                     CLASS_NAME,                 // Window class
                     "Learn to Program Windows", // Window text
                     WS_OVERLAPPEDWINDOW,        // Window style

                     // Size and position
                     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

                     NULL,      // Parent window
                     NULL,      // Menu
                     hInstance, // Instance handle
                     NULL       // Additional application data
      );

  if (hwnd == NULL) {
    return 0;
  }

  ShowWindow(hwnd, SW_NORMAL);

  // Run the message loop.

  MSG msg = {};
  while (GetMessage(&msg, NULL, 0, 0) > 0 && !st.stop_requested()) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

outcome::result<void> run_server(common::shared_memory *shm,
                                 std::stop_token st) {
  using namespace boost::interprocess;
  BOOST_LOG_TRIVIAL(info) << "Starting server";

  constexpr message_handler mh{};

  std::jthread log_thread(log_file_writer_thread);
  std::jthread wndthread(wnd_thread);

  while (true) {
    try {
      scoped_lock<interprocess_mutex> lk(shm->m_mut);
      shm->m_cv.wait(lk);

      auto target_msg = shm->m_buf.back();

      std::visit(mh, target_msg);
      shm->m_buf.pop_back();
    } catch (std::exception &ex) {
      BOOST_LOG_TRIVIAL(error) << "Exception in server: " << ex.what();

      log_thread.request_stop();
      wndthread.request_stop();

      return outcome::failure(std::error_code());
    }
  }

  log_thread.request_stop();
  wndthread.request_stop();

  BOOST_LOG_TRIVIAL(info) << "Exiting server successfully";
  return outcome::success();
}
