#pragma once

#include "MinHook.h"

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

		if (MH_CreateHook(old_func_ptr, new_func_ptr, &trampoline_ptr) != MH_OK) {
			return nullptr;
		}

		m_hooks.push_back(old_func_ptr);

		if (MH_EnableHook(old_func_ptr) != MH_OK) {
			return nullptr;
		}

		return reinterpret_cast<T>(trampoline_ptr);
	}

	bool disable_hook(void* func_ptr);
};
