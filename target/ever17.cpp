#include "ever17.h"
#include "base.h"
#include "msg.h"

#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>

using namespace targets::kid;

static std::shared_ptr<ever17> g_ever17 = nullptr;

common::shared_memory* targets::kid::ever17::get_shm_ptr() {
  void* p = m_region.get_address();
  return static_cast<common::shared_memory*>(p);
}

std::shared_ptr<ever17>
targets::kid::ever17::try_create(hook_manager &hm, boost::interprocess::mapped_region&& region) {
  std::filesystem::path module_path = common::get_module_file_name_w();

  auto filename = std::move(module_path).filename().wstring();
  std::transform(filename.begin(), filename.end(), filename.begin(), towupper);
  
  if (filename != L"EVER17PC.EXE") {
    return nullptr;
  }

  auto ptr = std::make_shared<ever17>();
  g_ever17 = ptr;
  
  ptr->m_region = std::move(region);
  ptr->m_textoutw_hook.enable(hm, textoutw_hook::fake_call);

  auto* shm_ptr = ptr->get_shm_ptr();
  send_message(shm_ptr, common::target_connected_message_t{});

  return ptr;
}

ever17::textoutw_hook::return_t WINAPI ever17::textoutw_hook::fake_call(HDC hdc, int x, int y, LPCSTR lpString, int c) {
  std::string out(lpString);
  uint32_t codepage = GetACP();

  auto* shm_ptr = g_ever17->get_shm_ptr();
  send_message(shm_ptr, common::target_generic_message_t {
      .m_text{lpString},
      .m_cp{codepage}
    });
  
  return g_ever17->m_textoutw_hook.m_old_ptr(hdc, x, y, lpString, c);
}

ever17::~ever17()
{
  auto* shm_ptr = get_shm_ptr();
  send_message(shm_ptr, common::target_close_message_t{});
  
  g_ever17 = nullptr;
}
