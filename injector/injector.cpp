#include "injector.h"

#include <string>
#include <filesystem>
#include <array>
#include <exception>
#include <source_location>

using namespace std;

constexpr auto nPipeBufferSize = 4096;

[[noreturn]] void throw_system_error(const std::string& msg, DWORD dwErrCode = GetLastError()) {
	throw std::system_error(dwErrCode, std::system_category(), msg);
}

void hookToProcess(DWORD dwPID, wstring const& dllName) {
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, dwPID);

	bool bRes = true;

	if (hProcess == NULL) {
		DWORD dwErrCode = GetLastError();
		if (dwErrCode == ERROR_ACCESS_DENIED)
		{
			// TODO : Elevate injector if target is elevated
		}
		else {
			throw_system_error("Failed to open process handle");
		}
		
		return;
	}

	size_t nLen = wcslen(dllName.c_str()) * sizeof(WCHAR);
	HINSTANCE hKernel32 = GetModuleHandleW(L"KERNEL32.DLL");

	if (hKernel32 == NULL) {
		throw_system_error("Failed to get KERNEL32.DLL handle");
		return;
	}

	void* lpLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
	if (!lpLoadLibraryW) {
		throw_system_error("Failed to get LoadLibraryW address");
		return;
	}

	void* lpRemoteString = VirtualAllocEx(hProcess, nullptr, nLen + 1, MEM_COMMIT, PAGE_READWRITE);
	if (lpRemoteString == NULL) {
		throw_system_error("Failed to allocate in target");
		return;
	}

	bRes = WriteProcessMemory(hProcess, lpRemoteString, dllName.c_str(), nLen, nullptr);
	if (!bRes) {
		throw_system_error("Could not write to process memory");
		return;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)lpLoadLibraryW, lpRemoteString, 0, nullptr);

	if (hThread) {
		WaitForSingleObject(hThread, 500);
	}

	VirtualFreeEx(hProcess, lpRemoteString, 0, MEM_RELEASE);

	CloseHandle(hProcess);

	return;
}

int main(int argc, char* argv[])
{
	string pid_str;
	filesystem::path dllPath = filesystem::current_path() / "../target/hookdict_target.dll";
	dllPath = filesystem::absolute(dllPath);
	
	if (!filesystem::exists(dllPath)) {
		cerr << "Target DLL not found" << endl;
		return 1;
	}

	HANDLE hPipe = CreateNamedPipeW(
		L"\\\\.\\pipe\\hookdict_pipe",
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS,
		1,
		0,
		0,
		0,
		nullptr
	);

	cout << "Created pipe" << endl;

	try {
		cout << "Process ID: ";
		cin >> pid_str;
		long pid = stol(pid_str);
		hookToProcess(pid, L"hookdict_target.dll");
	}
	catch (std::system_error const& ex) {
		cerr << "System error: " << ex.what();
		return 1;
	}
	catch ([[maybe_unused]] std::invalid_argument const& ex) {
		return 1;
	}

	if (hPipe == NULL || hPipe == INVALID_HANDLE_VALUE) {
		throw_system_error("Failed to open pipe to target");
		return 1;
	}

	while (true) {
		bool bRes = ConnectNamedPipe(hPipe, nullptr);

		if (!bRes) {
			DWORD dwErrCode = GetLastError();
			if (dwErrCode != ERROR_PIPE_CONNECTED) {
				CloseHandle(hPipe);
				return 1;
			}
		}

		array<wchar_t, nPipeBufferSize> buf;
		using buf_element = decltype(buf)::value_type;
		buf.fill('\0');

		DWORD nBytesRead = 0;
		bRes = ReadFile(hPipe,
			buf.data(),
			(buf.size() - 1) * sizeof(buf_element),
			&nBytesRead,
			nullptr);

		if (!bRes) {
			throw_system_error("Failed to read from pipe");
			CloseHandle(hPipe);
			return 1;
		}

		buf[nBytesRead / sizeof(buf_element)] = '\0';
		std::wcout << buf.data() << endl;
	}

	return 0;
}
 