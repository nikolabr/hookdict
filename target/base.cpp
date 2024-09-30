#include "base.h"

#include "msg.h"

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include <gdiplus.h>
#include <shlwapi.h>

#include <fstream>
#include <wingdi.h>
  
void send_message(common::shared_memory* ptr, common::message_t msg) {
  using namespace boost::interprocess;
  
  scoped_lock<interprocess_mutex> lk(ptr->m_mut);
  ptr->m_buf.push_back(msg);
  ptr->m_cv.notify_one();
}

IStream* get_img_buf_stream(common::shared_memory* shm_ptr) {
  auto& img_buf = shm_ptr->m_img_buf;

  return ::SHCreateMemStream(img_buf.m_buf.data(), img_buf.m_buf.size());
}
