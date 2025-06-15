#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <memory>
#include <string_view>
#include <cstdint>
#include <vector>

typedef struct _NULL_MEMORY {
	void* buffer_address;
	UINT_PTR address;
	ULONGLONG size;
	ULONG pid;
	BOOLEAN write;
	BOOLEAN read;
	BOOLEAN req_base;
	void* output;
	const char* module_name;
	ULONG64 base_address;
} NULL_MEMORY;

// global variables are bad
uintptr_t base_address = 0;
std::uint32_t process_id = 0;
static std::once_flag flag;

#pragma region Memory Management
// https://www.unknowncheats.me/forum/programming-for-beginners/333553-creating-performant-basic-external-cheat-using-modern-concepts.html
struct HandleDisposer
{
	using pointer = HANDLE; // this needs to be defined here so we can use void pointers

	// operator that gets called by our smart-pointer
	void operator()(HANDLE handle) const
	{
		if (handle != NULL || handle != INVALID_HANDLE_VALUE)
		{
			CloseHandle(handle);
		}
	}
};

using unique_handle = std::unique_ptr<HANDLE, HandleDisposer>;
#pragma endregion


template<typename ... Arg>
uint64_t call_hook(const Arg ... args) {
	std::call_once(flag, [] { LoadLibraryW(L"user32.dll"); });
	void* hooked_func = GetProcAddress(LoadLibraryW(L"win32u.dll"), "NtOpenCompositionSurfaceSectionInfo");

	printf("func addr: %p\n", hooked_func);
	const auto func = static_cast<uint64_t(_stdcall*)(Arg...)>(hooked_func);

	return func(args...);
}

static std::uint32_t get_process_id(std::string_view process_name) {
	PROCESSENTRY32 process_entry;
	const unique_handle snapshot_handle(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL));

	if (snapshot_handle.get() == INVALID_HANDLE_VALUE)
		return NULL;

	process_entry.dwSize = sizeof(MODULEENTRY32);
	while (Process32Next(snapshot_handle.get(), &process_entry) == TRUE) {
		if (process_name.compare(process_entry.szExeFile) == NULL)
			return process_entry.th32ProcessID;
	}

	return NULL;
}

static ULONG64 get_module_base_address(const char* module_name) {
	NULL_MEMORY instructions = { 0 };
	instructions.pid = process_id;
	instructions.req_base = TRUE;
	instructions.read = FALSE;
	instructions.write = FALSE;
	instructions.module_name = module_name;
	call_hook(&instructions);

	ULONG64 base = NULL;
	base = instructions.base_address;
	return base;
}

template <class T>
T Read(UINT_PTR read_address) {
	T response{};
	NULL_MEMORY instructions;
	instructions.pid = process_id;
	instructions.size = sizeof(T);
	instructions.address = read_address;
	instructions.read = TRUE;
	instructions.write = FALSE;
	instructions.req_base = FALSE;
	instructions.output = &response;
	call_hook(&instructions);

	return response;
}

bool write_memory(UINT_PTR write_address, UINT_PTR source_address, SIZE_T write_size) {
	NULL_MEMORY instructions;
	instructions.pid = process_id;
	instructions.size = write_size;
	instructions.address = write_address;
	instructions.read = FALSE;
	instructions.write = TRUE;
	instructions.req_base = FALSE;
	instructions.buffer_address = (void*)source_address;
	call_hook(&instructions);

	return true;
}

template<typename S>
bool write(UINT_PTR write_address, const S& value) {
	write_memory(write_address, (UINT_PTR)&value, sizeof(S));
}

int main() {
	printf("Running\n");

	base_address = get_module_base_address("Explorer.exe");
	if (!base_address)
		printf("base address not found\n");
	else
		printf("Found base address\n");

	Sleep(5000);
	return 0;
}