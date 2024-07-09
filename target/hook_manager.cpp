#include "hook_manager.h"

#include <stdexcept>

bool hook_manager::init()
{
	if (MH_Initialize() != MH_OK) {
		return false;
	}
	return true;
}

bool hook_manager::uninit()
{
	if (MH_Uninitialize() != MH_OK) {
		return false;
	}
	return true;
}

bool hook_manager::disable_hook(void* func_ptr) {
	m_hooks.remove(func_ptr);

	if (MH_DisableHook(func_ptr) != MH_OK) {
		return false;
	}
	return true;
}