#pragma once

#include "common.h"

#include <variant>

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition_any.hpp>
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

  using message_t = std::variant<
    target_connected_message_t,
    target_close_message_t,
    target_generic_message_t
    >;

  struct shared_image_buf {
    std::array<uint8_t, 1024 * 1024 * 8> m_buf{};
  };
  
  struct shared_memory {
    static constexpr std::size_t buf_capacity = 512;
    
    boost::container::static_vector<message_t, buf_capacity> m_buf;

    boost::interprocess::interprocess_mutex m_mut;
    
    boost::interprocess::interprocess_condition_any m_cv;
  };
}
