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

	HANDLE get_process_id(const char* process_name) {
		ULONG buffer_size = 0;
		ZwQuerySystemInformation(SystemProcessInformation, NULL, NULL, &buffer_size);

		auto buffer = ExAllocatePoolWithTag(NonPagedPool, buffer_size, 'tag2');
		if (!buffer) {
			KdPrint(0, 0, "failed to allocate pool (get_process_id)");
			return 0;
		}

		ANSI_STRING process_name_ansi = {};
		UNICODE_STRING process_name_unicode = {};
		RtlInitAnsiString(&process_name_ansi, process_name);
		if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&process_name_unicode, &process_name_ansi, TRUE))) {
			KdPrint(0, 0, "failed to convert string (get_process_id)");
			RtlFreeUnicodeString(&process_name_unicode);
			return 0;
		}

		auto process_info = (PSYSTEM_PROCESS_INFO)buffer;
		if (NT_SUCCESS(ZwQuerySystemInformation(SystemProcessInformation, process_info, buffer_size, NULL))) {
			while (process_info->NextEntryOffset) {
				if (!RtlCompareUnicodeString(&process_name_unicode, &process_info->ImageName, true)) {
					KdPrint(0, 0, "process name: %wZ | process ID: %d\n", process_info->ImageName, process_info->UniqueProcessId);
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
			KdPrint(0, 0, "process lookup failed (read)");
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
			KdPrint(0, 0, "process lookup failed (write)");
			return false;
		}

		return MmCopyVirtualMemory(PsGetCurrentProcess(), address, process, buffer, size, KernelMode, &bytes) == STATUS_SUCCESS;
	}
}