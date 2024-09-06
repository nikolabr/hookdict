#include "ever17.h"
#include "base.h"
#include "wil/result_macros.h"

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "mbstring.h"

using namespace targets::kid;

static std::shared_ptr<ever17> g_ever17 = nullptr;

std::shared_ptr<ever17>
targets::kid::ever17::try_create(hook_manager &hm, wil::shared_hfile pipe) {
  std::filesystem::path module_path = common::get_module_file_name_w();

  auto filename = std::move(module_path).filename().wstring();
  std::transform(filename.begin(), filename.end(), filename.begin(), towupper);

  if (filename != L"EVER17PC.EXE") {
    write_to_pipe(pipe,"Target is not ever17\n");
    return nullptr;
  }

  auto ptr = std::make_shared<ever17>();
  ptr->m_pipe = pipe;
  g_ever17 = ptr;
  
  ptr->m_textoutw_hook.enable(hm, textoutw_hook::fake_call);

  return ptr;
}

// __attribute__((stdcall)) BOOL ever17::fake_TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c) {
//   write_to_pipe(g_pipe, lpString);
//   return g_ever17->m_original_textoutw(hdc, x, y, lpString, c);
// }

ever17::textoutw_hook::return_t WINAPI ever17::textoutw_hook::fake_call(HDC hdc, int x, int y, LPCSTR lpString, int c) {
  // std::ostringstream os;
  // os << "Device context handle: " << (void*)hdc << " ";
  // os << "X: " << x << " ";
  // os << "Y: " << y << " ";
  // os << "Text: " << lpString << " ";
  // os << "c: " << c << " ";
  // os << std::endl;
  
  // std::string out_msg = os.str();
  // write_to_pipe(g_ever17->m_pipe, std::move(out_msg));

  std::string cur(lpString);

  if (cur != g_ever17->m_prev) {
    write_to_pipe(g_ever17->m_pipe, cur);  
  }
  g_ever17->m_prev = std::move(cur);
  
  return g_ever17->m_textoutw_hook.m_old_ptr(hdc, x, y, lpString, c);
}

ever17::~ever17()
{
  g_ever17 = nullptr;
}
