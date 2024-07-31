#include "process_info.h"

#include <iterator>
#include <array>
#include <algorithm>

#include <Windows.h>
#include <Psapi.h>

#include <wil/result_macros.h>
#include <wil/resource.h>

constexpr std::size_t max_process_to_enumerate = 1024;

std::wstring process_info::get_process_name(uint32_t pid)
{
	std::array<wchar_t, MAX_PATH> buf;

	wil::unique_process_handle proc_handle(
		OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, pid)
	);
	if (proc_handle == NULL)
	{
		return L"";
	}

	DWORD res = GetModuleFileNameExW(proc_handle.get(), nullptr, buf.data(), buf.size());
	THROW_LAST_ERROR_IF(res == 0);

	return std::wstring(buf.data(), res);
}

std::vector<process_info> process_info::enum_processes()
{
	std::array<DWORD, 1024> processes_arr;

	DWORD returned = 0;
	DWORD dwRes = EnumProcesses(processes_arr.data(), processes_arr.size(), &returned);
	THROW_LAST_ERROR_IF(dwRes == 0);

	std::vector<process_info> res;

	std::transform(processes_arr.begin(), processes_arr.begin() + returned, std::back_inserter(res),
		[](auto&&... args) {
			return process_info(args...);
		});
	
	return res;
}
