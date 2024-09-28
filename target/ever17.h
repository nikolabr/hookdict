#pragma once

#include "base.h"
#include "common.h"
#include "hook_manager.h"

#include <boost/interprocess/mapped_region.hpp>
#include <type_traits>

namespace targets {
  namespace kid {
    class ever17
    {
      std::string m_prev;
      
      boost::interprocess::mapped_region m_region;
    public:
      struct textoutw_hook : hooks::hook_base<decltype(&TextOutA), HDC, int, int, LPCSTR, int> {
	using hook_base::hook_base;
	using return_t = hook_base::return_t;
	
	static return_t WINAPI fake_call(HDC hdc, int x, int y, const char *lpString, int c);
      };

      common::shared_memory* get_shm_ptr();
      
      HBITMAP m_hbm = NULL;
      std::atomic<HWND> m_main_wnd = NULL;

      static std::shared_ptr<ever17> try_create(hook_manager& hm, boost::interprocess::mapped_region&& region);
      
      ~ever17();
    private:
      textoutw_hook m_textoutw_hook{TextOutA};
    };
  }
}
