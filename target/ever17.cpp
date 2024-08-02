#include "ever17.h"
#include "base.h"
#include "wil/result_macros.h"

#include <cstdlib>
#include <cstring>
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
  ptr->m_original_textoutw = hm.enable_hook(TextOutA, &fake_TextOutA);

  // wil::unique_hmodule module(GetModuleHandleW(L"GDI32.DLL"));
  // LOG_LAST_ERROR_IF_NULL(module);

  // FARPROC original_text_out_w = ::GetProcAddress(module.get(), "TextOutW");
  // LOG_LAST_ERROR_IF_NULL(original_text_out_w);

  // ptr->m_original_textoutw = hm.enable_hook(reinterpret_cast<ever17::TextOutW_t>(original_text_out_w), &fake_TextOutW);
  LOG_IF_NULL_ALLOC_MSG(ptr->m_original_textoutw, "Failed to create TextOutW hook\n");

  return ptr;
}

__attribute__((stdcall)) BOOL ever17::fake_TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c) {
  write_to_pipe(g_pipe, lpString);
  return g_ever17->m_original_textoutw(hdc, x, y, lpString, c);
}

ever17::~ever17()
{
  g_ever17 = nullptr;
}
