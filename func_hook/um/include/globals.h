#pragma once
#include <Windows.h>

namespace globals {
	inline HANDLE process_id = NULL;
	inline uintptr_t module_base = NULL;
	inline uintptr_t peb_address = NULL;
}