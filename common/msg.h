#pragma once

#include "common.h"

#include <variant>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition_any.hpp>
#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <boost/container/static_vector.hpp>
#include <boost/static_string.hpp>

namespace common {
  static const char* shared_memory_name = "hookdict_shared_memory";

  struct target_connected_message_t {
    boost::static_string<128> m_window_name;
  };

  struct target_generic_message_t {
    boost::static_string<128> m_text;
    uint32_t m_cp;
  };

  struct target_close_message_t {};

  struct bitblt_hook_message_t {
    struct {
      HDC hdc;
      int x;
      int y;
      int cx;
      int cy;
      HDC hdcSrc;
      int x1;
      int y1;
      DWORD rop;
    } m_args;
    
    std::string m_src_dc_text;
  };

  using message_t = std::variant<
    target_connected_message_t,
    target_close_message_t,
    target_generic_message_t,
    bitblt_hook_message_t
    >;

  struct shared_image_buf {
    std::array<unsigned char, 1024 * 1024 * 8> m_buf;
    
    boost::interprocess::interprocess_semaphore m_sem{1};
  };
  
  struct shared_memory {
    static constexpr std::size_t buf_capacity = 512;

    shared_image_buf m_img_buf;
    
    boost::container::static_vector<message_t, buf_capacity> m_buf;

    boost::interprocess::interprocess_mutex m_mut;
    
    boost::interprocess::interprocess_condition_any m_cv;
  };
}
