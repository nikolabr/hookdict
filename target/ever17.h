#pragma once

#include "base.h"
#include "hook_manager.h"

#include <type_traits>

namespace targets {
  namespace kid {
    class ever17
    {
      wil::shared_hfile m_pipe;

      using TextOutA_t = decltype(&TextOutA);
      TextOutA_t m_original_textoutw = nullptr;

      static __attribute__((stdcall)) BOOL fake_TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c);
      static_assert(std::is_same_v<decltype(&fake_TextOutA), TextOutA_t>);
      
    public:
      static constexpr wchar_t s_target_name[] = L"kid::ever17";

      static std::shared_ptr<ever17> try_create(hook_manager& hm, wil::shared_hfile pipe);

      ~ever17();
    };
  }
}
