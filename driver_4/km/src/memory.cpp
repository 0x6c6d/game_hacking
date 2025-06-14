#include "memory.h"

PVOID get_system_module_base(const char* module_name) {
	ULONG bytes = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, NULL, bytes, &bytes);
	if (!bytes)
		return NULL;

	PSYSTEM_MODULE_INFORMATION modules = (PSYSTEM_MODULE_INFORMATION)ExAllocatePoolWithTag(NonPagedPool, bytes, 'tag');
	status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes);
	if (!NT_SUCCESS(status))
		return NULL;

	PSYSTEM_MODULE_ENTRY module = modules->Modules;
	PVOID module_base = 0, module_size = 0;

	for (ULONG i = 0; i < modules->NumberOfModules; i++) {
		if (strcmp((char*)module[i].FullPathName, module_name) == NULL) {
			// module found
			module_base = module[i].ImageBase;
			module_size = (PVOID)module[i].ImageSize;
			break;
		}
	}

	if (modules)
		ExFreePoolWithTag(modules, NULL);

	if (module_base == NULL)
		return NULL;

	return module_base;
}

PVOID get_system_module_export(const char* module_name, LPCSTR routine_name) {
	PVOID lp_module = get_system_module_base(module_name);
	if (!lp_module)
		return NULL;

	return RtlFindExportedRoutineByName(lp_module, routine_name);
}

bool write_memory(void* address, void* buffer, size_t size) {
	if (!RtlCopyMemory(address, buffer, size))
		return false;

	return true;
}

bool write_to_read_only_memory(void* address, void* buffer, size_t size) {
	PMDL mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);
	if (!mdl)
		return false;

	MmProbeAndLockPages(mdl, KernelMode, IoReadAccess);
	PVOID mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
	MmProtectMdlSystemAddress(mdl, PAGE_READWRITE);

	write_memory(mapping, buffer, size);

	MmUnmapLockedPages(mapping, mdl);
	MmUnlockPages(mdl);
	IoFreeMdl(mdl);

	return true;
}