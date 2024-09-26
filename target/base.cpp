#include "base.h"

#include "msg.h"

#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

void send_message(common::shared_memory* ptr, common::message_t msg) {
  using namespace boost::interprocess;
  
  scoped_lock<interprocess_mutex> lk(ptr->m_mut);
  ptr->m_buf.push_back(msg);
  ptr->m_cv.notify_one();
}
