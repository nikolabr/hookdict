#include "ever17.h"
#include "base.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>

using namespace targets::kid;

static std::shared_ptr<ever17> g_ever17 = nullptr;

std::shared_ptr<ever17>
targets::kid::ever17::try_create(hook_manager &hm) {
  std::filesystem::path module_path = common::get_module_file_name_w();

  auto filename = std::move(module_path).filename().wstring();
  std::transform(filename.begin(), filename.end(), filename.begin(), towupper);
  
  if (filename != L"EVER17PC.EXE") {
    return nullptr;
  }

  auto ptr = std::make_shared<ever17>();
  g_ever17 = ptr;
  
  ptr->m_textoutw_hook.enable(hm, textoutw_hook::fake_call);

  return ptr;
}

ever17::textoutw_hook::return_t WINAPI ever17::textoutw_hook::fake_call(HDC hdc, int x, int y, LPCSTR lpString, int c) {
  std::string out(lpString);
  uint32_t codepage = GetACP();
  
  return g_ever17->m_textoutw_hook.m_old_ptr(hdc, x, y, lpString, c);
}

ever17::~ever17()
{
  g_ever17 = nullptr;
}
