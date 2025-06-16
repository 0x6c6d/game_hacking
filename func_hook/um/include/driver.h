#pragma once
#include <Windows.h>
#include <cstdint>
#include <memory>
#include <string_view>
#include <TlHelp32.h>
#include <mutex>
#include <iostream>
#include "globals.h"

typedef struct _COPY_MEMORY {
	void* buffer;
	ULONG64		address;
	ULONG		size;
	HANDLE		pid;
	bool		get_pid;
	bool		base;
	bool		peb;
	bool		read;
	bool		write;
	bool		get_client;
	bool		get_engine;
	bool		get_engine_size;
	bool		get_client_size;
	const char* module_name;
	const char* process_name;
} COPY_MEMORY;

namespace driver
{
	static std::once_flag flag;

	template<typename ... A>
	uint64_t call_hook(const A ... arguments)
	{
		std::call_once(flag, [] { LoadLibrary(L"user32.dll"); });
		void* control_function = GetProcAddress(LoadLibrary(L"win32u.dll"), "NtOpenCompositionSurfaceSectionInfo");
		const auto control = static_cast<uint64_t(__stdcall*)(A...)>(control_function);

		return control(arguments ...);
	}

	static HANDLE get_process_id(const char* process_name) 
	{
		COPY_MEMORY m{};
		m.get_pid = true;
		m.process_name = process_name;	
		call_hook(&m);
		return m.pid;
	}

	static uintptr_t get_module_base(HANDLE pid, const char* mod_name)
	{
		COPY_MEMORY m{};
		m.base = true;
		m.pid = pid;
		m.module_name = mod_name;
		call_hook(&m);
		return (uintptr_t)m.buffer;
	}

	static uintptr_t get_module_info(HANDLE pid, const char* mod_name)
	{
		COPY_MEMORY m{};
		m.base = true;
		m.pid = pid;
		m.module_name = mod_name;
		call_hook(&m);

		return (uintptr_t)m.buffer;
	}

	static uintptr_t get_peb_address(HANDLE pid)
	{
		COPY_MEMORY m{};
		m.peb = true;
		m.pid = pid;
		call_hook(&m);
		return (uintptr_t)m.buffer;
	}

	static uintptr_t get_client(HANDLE pid)
	{
		COPY_MEMORY m{};
		m.get_client = true;
		m.pid = pid;
		call_hook(&m);
		return (uintptr_t)m.buffer;
	}

	static uintptr_t get_engine(HANDLE pid)
	{
		COPY_MEMORY m{};
		m.get_engine = true;
		m.pid = pid;
		call_hook(&m);
		return (uintptr_t)m.buffer;
	}

	static ULONG get_engine_size(HANDLE pid)
	{
		COPY_MEMORY m{};
		m.get_engine_size = true;
		m.pid = pid;
		call_hook(&m);
		return (ULONG)m.size;
	}

	static ULONG get_client_size(HANDLE pid)
	{
		COPY_MEMORY m{};
		m.get_client_size = true;
		m.pid = pid;
		call_hook(&m);
		return (ULONG)m.size;
	}

	template<typename type>
	type rpm(uintptr_t read_addr)
	{
		type buffer{};

		COPY_MEMORY m{};
		m.read = true;
		m.pid = globals::process_id;
		m.address = read_addr;
		m.buffer = &buffer;
		m.size = sizeof(type);	
		call_hook(&m);
		return buffer;
	}

	template<typename type>
	void wpm(UINT_PTR write_addr, type value)
	{
		COPY_MEMORY m{};
		m.write = true;
		m.pid = globals::process_id;
		m.address = write_addr;
		m.buffer = &value;
		m.size = sizeof(value);
		call_hook(&m);
	}
}