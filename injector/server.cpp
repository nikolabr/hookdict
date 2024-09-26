#include "server.h"
#include "common.h"
#include "msg.h"

#include <thread>

#include <boost/log/trivial.hpp>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <boost/winapi/character_code_conversion.hpp>
#include <boost/lockfree/queue.hpp>

static boost::lockfree::queue<wchar_t> text_queue(4096);

outcome::result<std::wstring> on_target_msg(auto const &text, uint32_t codepage) {
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
  auto operator()(common::target_connected_message_t const&) const {
    BOOST_LOG_TRIVIAL(info) << "Target connected";
  };
  
  auto operator()(common::target_generic_message_t const& msg) const {
    auto wstr = on_target_msg(msg.m_text, msg.m_cp);
    if (wstr.has_value()) {
      for (const wchar_t c : wstr.value()) {
	text_queue.push(c);
      }
    }
  };

  auto operator()(common::target_close_message_t const&) const {
    BOOST_LOG_TRIVIAL(info) << "Target closed";
    throw std::runtime_error("Target closed");
  }
};

static outcome::result<void> log_file_writer_thread(std::stop_token st) {
    HANDLE log_file = ::CreateFile("target.log",
				 GENERIC_READ | GENERIC_WRITE,
				 FILE_SHARE_READ | FILE_SHARE_WRITE,
				 nullptr,
				 OPEN_EXISTING,
				 FILE_ATTRIBUTE_NORMAL, NULL);

  if (log_file != INVALID_HANDLE_VALUE) {
    BOOST_LOG_TRIVIAL(warning) << "Failed to open log file";
  }

  while (!st.stop_requested()) {
    text_queue.consume_all([&](const wchar_t c) {
      ::WriteFile(log_file, &c, sizeof(wchar_t), nullptr, nullptr);
    });
    ::FlushFileBuffers(log_file);
  }
  
  ::CloseHandle(log_file);
  return outcome::success();
}

outcome::result<void> run_server(common::shared_memory* shm, std::stop_token st) {
  using namespace boost::interprocess;
  BOOST_LOG_TRIVIAL(info) << "Starting server";

  constexpr message_handler mh{};
  std::jthread log_thread(log_file_writer_thread);
  
  while (true) {
    try {
      scoped_lock<interprocess_mutex> lk(shm->m_mut);
      shm->m_cv.wait(lk);
    
      BOOST_LOG_TRIVIAL(debug) << "Received message";

      auto msg = shm->m_buf.back();
      
      std::visit(mh, msg);
      shm->m_buf.pop_back();
    }
    catch (std::exception& ex) {
      BOOST_LOG_TRIVIAL(error) << "Exception in server: " << ex.what();
      return outcome::failure(std::error_code());
    }
  }
  
  BOOST_LOG_TRIVIAL(info) << "Exiting server successfully";
  return outcome::success();
}
