#pragma once
#include "common.h"

#include "MinHook.h"
#include "base.h"

#include <cstdlib>
#include <list>

class hook_manager
{
	std::list<void*> m_hooks;
public:
	bool init();
	bool uninit();

	template <typename T>
	T enable_hook(T old_func_ptr, T new_func_ptr) {
		void* trampoline_ptr = nullptr;
		void* old_ptr = reinterpret_cast<void*>(old_func_ptr);

		MH_STATUS st;
		const auto report_error = [&]() {
		  write_to_pipe(g_pipe, MH_StatusToString(st));
		};

		st = MH_CreateHook(old_ptr, reinterpret_cast<void*>(new_func_ptr), &trampoline_ptr);
		if (st != MH_OK) {
		  report_error();
		  return nullptr;
		}

		m_hooks.push_back(old_ptr);
		st = MH_EnableHook(old_ptr);
		if (st != MH_OK) {
		  report_error();
		  return nullptr;
		}
		
		return reinterpret_cast<T>(trampoline_ptr);
	}

	bool disable_hook(void* func_ptr);
};
