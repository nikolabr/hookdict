#include "server.h"
#include "common.h"
#include "msg.h"

#include <fstream>
#include <thread>
#include <vector>
#include <chrono>

#include <boost/log/trivial.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/lockfree/queue.hpp>
#include <boost/winapi/character_code_conversion.hpp>

static boost::lockfree::queue<wchar_t> text_queue(4096);

outcome::result<std::wstring> cp932_to_utf16(auto const &text,
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
    auto wstr = cp932_to_utf16(msg.m_text, msg.m_cp);
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

static void log_file_writer_thread(std::stop_token st) {
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
  return;
}

static void image_buf_reader_thread(std::stop_token st, common::shared_image_buf* buf_ptr) {
  int counter = 0;
  
  std::vector<unsigned char> sib;
  
  while (!st.stop_requested()) {
    while(!buf_ptr->m_sem.try_wait()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
    };
    
    BOOST_LOG_TRIVIAL(debug) << "Img buf update";
    sib = std::vector(buf_ptr->m_buf.begin(), buf_ptr->m_buf.end());

    buf_ptr->m_sem.post();
    
    std::ostringstream ss;
    ss << "image" << counter << ".bmp";
    counter++;
    
    std::ofstream fs(ss.str(), std::ios::binary | std::ios::trunc);
    fs.write(reinterpret_cast<char*>(sib.data()), sib.size());
  }
}

outcome::result<void> run_server(common::shared_memory *shm,
                                 std::stop_token st) {
  using namespace boost::interprocess;
  BOOST_LOG_TRIVIAL(info) << "Starting server";

  message_handler mh{};
  mh.m_ptr = shm;

  std::array<std::jthread, 2> server_threads = {
    std::jthread(log_file_writer_thread),
    
    std::jthread([shm](std::stop_token st) {
      return image_buf_reader_thread(st, &shm->m_img_buf);
    })
  };

  const auto fn_request_thread_stop = [&]() {
    for (auto& t : server_threads) {
      t.request_stop();
    }
  };

  while (true) {
    try {
      scoped_lock<interprocess_mutex> lk(shm->m_mut);
      shm->m_cv.wait(lk);

      auto target_msg = shm->m_buf.back();
      
      std::visit(mh, target_msg);
      shm->m_buf.pop_back();
    } catch (std::exception &ex) {
      BOOST_LOG_TRIVIAL(error) << "Exception in server: " << ex.what();

      fn_request_thread_stop();

      return outcome::failure(std::error_code());
    }
  }

  fn_request_thread_stop();
  
  BOOST_LOG_TRIVIAL(info) << "Exiting server successfully";
  return outcome::success();
}
