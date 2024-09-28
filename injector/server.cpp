#include "server.h"
#include "common.h"
#include "msg.h"

#include <fstream>
#include <thread>

#include <boost/log/trivial.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/lockfree/queue.hpp>
#include <boost/winapi/character_code_conversion.hpp>

static boost::lockfree::queue<wchar_t> text_queue(4096);

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
  common::shared_memory* m_ptr{};
  unsigned int m_counter = 0;
  
  auto operator()(common::target_connected_message_t const &msg) const {
    BOOST_LOG_TRIVIAL(info) << "Target connected";
    BOOST_LOG_TRIVIAL(info) << "Window name: " << msg.m_window_name;
    // HWND hwnd = ::FindWindowA(nullptr, msg.m_window_name.c_str());
    // if (hwnd == NULL) {
    //   BOOST_LOG_TRIVIAL(warning) << "Target window not found";
    // }
    // target_hwnd = hwnd;
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

  auto operator()(common::bitmap_update_message_t const& msg) {
    BOOST_LOG_TRIVIAL(info) << "Bitmap updated " <<
      msg.m_width << " " <<
      msg.m_height << " " <<
      msg.m_lines;
    
    std::ostringstream ss;
    ss << "screenshot" << m_counter << ".bmp";
    std::ofstream ofs(ss.str(), std::ios::binary | std::ios::trunc);
    m_counter++;
    
    m_ptr->m_img_buf.m_sem.wait();
    ofs.write(m_ptr->m_img_buf.m_buf.data(), m_ptr->m_img_buf.m_buf.size());
    m_ptr->m_img_buf.m_sem.post();
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

outcome::result<void> run_server(common::shared_memory *shm,
                                 std::stop_token st) {
  using namespace boost::interprocess;
  BOOST_LOG_TRIVIAL(info) << "Starting server";

  message_handler mh{};
  mh.m_ptr = shm;

  std::jthread log_thread(log_file_writer_thread);

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

      return outcome::failure(std::error_code());
    }
  }

  log_thread.request_stop();
  
  BOOST_LOG_TRIVIAL(info) << "Exiting server successfully";
  return outcome::success();
}
