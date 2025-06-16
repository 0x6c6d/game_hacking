#pragma once
#include "imports.h"

namespace memory {
	PVOID get_system_module_base(const char* module_name) {
		ULONG bytes = 0;
		NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes);
		if (!bytes)
			return 0;
		
		PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, 'tag1');

		status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes);
		if (!NT_SUCCESS(status))
			return 0;

		PRTL_PROCESS_MODULE_INFORMATION module = modules->Modules;
		PVOID module_base = 0, module_size = 0;

		for (ULONG i = 0; i < modules->NumberOfModules; i++)
		{
			if (strcmp((char*)module[i].FullPathName, module_name) == 0)
			{
				module_base = module[i].ImageBase;
				module_size = (PVOID)module[i].ImageSize;
				break;
			}
		}

		if (modules)
			ExFreePoolWithTag(modules, 0);

		return module_base;
	}

	ULONG64 get_module_base_x64(PEPROCESS proc) {
		return (ULONG64)PsGetProcessSectionBaseAddress(proc);
	}

	ULONG64 get_module_info(PEPROCESS proc, UNICODE_STRING module_name, BOOL get_size) {
		PCWSTR mod_name = module_name.Buffer;
		UNICODE_STRING module_name;
		RtlInitUnicodeString(&module_name, mod_name);

		PPEB peb = (PPEB)PsGetProcessPeb(proc);
		if (!peb)
			return 0;

		// attach to the process address space
		//		required to safely access um memory from km
		KAPC_STATE state;
		KeStackAttachProcess(proc, &state);

		// access the list of loaded modules
		PPEB_LDR_DATA ldr = (PPEB_LDR_DATA)peb->Ldr;
		if (!ldr) {
			KeUnstackDetachProcess(&state);
			return 0;
		}

		UNICODE_STRING name;

		// loop linked list
		for (PLIST_ENTRY list_entry = (PLIST_ENTRY)ldr->InLoadOrderModuleList.Flink;
			list_entry != &ldr->InLoadOrderModuleList;
			list_entry = (PLIST_ENTRY)list_entry->Flink) {
			PLDR_DATA_TABLE_ENTRY pEntry =
				CONTAINING_RECORD(list_entry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
			if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) ==
				0) {
				ULONG64 base_addr = (ULONG64)pEntry->DllBase;
				ULONG64 size = (ULONG64)pEntry->SizeOfImage;
				KeUnstackDetachProcess(&state);

				if (get_size)
					return size;

				return base_addr;
			}
		}

		// cleanup
		KeUnstackDetachProcess(&state);
		return 0;
	}

	HANDLE get_process_id(const char* process_name) {
		ULONG buffer_size = 0;
		ZwQuerySystemInformation(SystemProcessInformation, NULL, NULL, &buffer_size);

		auto buffer = ExAllocatePoolWithTag(NonPagedPool, buffer_size, 'tag2');
		if (!buffer) {
			DbgPrintEx(0, 0, "failed to allocate pool (get_process_id)");
			return 0;
		}

		ANSI_STRING process_name_ansi = {};
		UNICODE_STRING process_name_unicode = {};
		RtlInitAnsiString(&process_name_ansi, process_name);
		if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&process_name_unicode, &process_name_ansi, TRUE))) {
			DbgPrintEx(0, 0, "failed to convert string (get_process_id)");
			RtlFreeUnicodeString(&process_name_unicode);
			return 0;
		}

		auto process_info = (PSYSTEM_PROCESS_INFO)buffer;
		if (NT_SUCCESS(ZwQuerySystemInformation(SystemProcessInformation, process_info, buffer_size, NULL))) {
			while (process_info->NextEntryOffset) {
				if (!RtlCompareUnicodeString(&process_name_unicode, &process_info->ImageName, true)) {
					DbgPrintEx(0, 0, "process name: %wZ | process ID: %d\n", process_info->ImageName, process_info->UniqueProcessId);
					RtlFreeUnicodeString(&process_name_unicode);
					return process_info->UniqueProcessId;
				}
				process_info = (PSYSTEM_PROCESS_INFO)((BYTE*)process_info + process_info->NextEntryOffset);
			}
		}
		else {
			ExFreePoolWithTag(buffer, 'tag2');
			return 0;
		}
	}

	PVOID get_system_module_export(const char* module_name, LPCSTR routine_name)
	{
		PVOID lpModule = memory::get_system_module_base(module_name);

		if (!lpModule)
			return NULL;

		return RtlFindExportedRoutineByName(lpModule, routine_name);
	}

	PVOID get_client_dll(PEPROCESS proc, const char* module_name) {
		DbgPrintEx(0, 0, "[i] get_client_dll() requested: %s", module_name);
		PVOID module_base = NULL;
		ULONG size = 0;
		ZwQuerySystemInformation(SystemModuleInformation, &module_base, size, &size);
		module_base = ExAllocatePool(NonPagedPool, size);
		if (module_base == NULL)
			return NULL;
		if (!NT_SUCCESS(ZwQuerySystemInformation(SystemModuleInformation, module_base, size, &size)))
		{
			ExFreePool(module_base);
			return NULL;
		}
		PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)module_base;
		for (ULONG i = 0; i < modules->NumberOfModules; i++)
		{
			PRTL_PROCESS_MODULE_INFORMATION module = &modules->Modules[i];

			DbgPrintEx(0, 0, "[i] get_client_dll() found this: %s", module->FullPathName);
			if (strstr((char*)module->FullPathName, module_name) || strstr((char*)module->FullPathName, "client"))
			{
				ExFreePool(module_base);
				return module->ImageBase;
			}
		}
		ExFreePool(module_base);
		return NULL;

	}

	bool write_to_read_only_memory(void* address, void* buffer, size_t size) {
		PMDL mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);
		if (!mdl)
			return false;

		// locking and mapping memory with RW-rights
		MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
		PVOID mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
		MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);

		// write buffer to mapping
		RtlCopyMemory(mapping, buffer, size);

		// resources freeing
		MmUnmapLockedPages(mapping, mdl);
		MmUnlockPages(mdl);
		IoFreeMdl(mdl);

		return true;
	}

	bool read_kernel_memory(HANDLE pid, PVOID address, PVOID buffer, SIZE_T size) {
		if (!address || !buffer || !size)
			return false;

		SIZE_T bytes = 0;
		PEPROCESS process;
		if (!NT_SUCCESS(PsLookupProcessByProcessId(pid, &process))) {
			DbgPrintEx(0, 0, "process lookup failed (read)");
			return false;
		}

		return MmCopyVirtualMemory(process, address, PsGetCurrentProcess(), buffer, size, KernelMode, &bytes) == STATUS_SUCCESS;
	}

	bool write_kernel_memory(HANDLE pid, PVOID address, PVOID buffer, SIZE_T size) {
		if (!address || !buffer || !size)
			return false;

		SIZE_T bytes = 0;
		PEPROCESS process;
		if (!NT_SUCCESS(PsLookupProcessByProcessId(pid, &process))) {
			DbgPrintEx(0, 0, "process lookup failed (write)");
			return false;
		}

		return MmCopyVirtualMemory(PsGetCurrentProcess(), address, process, buffer, size, KernelMode, &bytes) == STATUS_SUCCESS;
	}
}