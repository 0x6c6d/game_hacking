#include "hook.h"

bool nullhook::call_kernel_function(void* kernel_function_address) {
	if (!kernel_function_address)
		return false;

	PVOID* func = reinterpret_cast<PVOID*>(get_system_module_export("\\SystemRoot\\System32\\drivers\\dxgkrnl.sys", "NtOpenCompositionSurfaceSectionInfo"));
	if (!func)
		return false;

	// first 12 bytes of the "NtOpenCompositionSurfaceSectionInfo" function -> WinDbg Disassembly
	BYTE orig[] = { 0x48, 0x8b, 0xc4, 0x4c, 0x89, 0x48, 0x20, 0x48, 0x89, 0x48, 0x08, 0x53 };

	// change signature of shellcode to avoid ac detection
	BYTE shell_code[] = { 0x48, 0xB8 }; // mov rax, <function_address>
	BYTE shell_code_end[] = { 0xFF, 0xE0 }; // jmp rax

	RtlSecureZeroMemory(&orig, sizeof(orig));
	memcpy((PVOID)((ULONG_PTR)orig), &shell_code, sizeof(shell_code));
	uintptr_t hook_address = reinterpret_cast<uintptr_t>(kernel_function_address);
	memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code)), &hook_address, sizeof(void*));
	memcpy((PVOID)((ULONG_PTR)orig + sizeof(shell_code) + sizeof(void*)), &shell_code_end, sizeof(shell_code_end));

	write_to_read_only_memory(func, &orig, sizeof(orig));

	return true;
}

NTSTATUS nullhook::hook_handler(PVOID called_param) {

	NULL_MEMORY* instructions = (NULL_MEMORY*)called_param;

	if (instructions->req_base != FALSE){
		ANSI_STRING AS;
		UNICODE_STRING ModuleName;

		RtlInitAnsiString(&AS, instructions->module_name);
		RtlAnsiStringToUnicodeString(&ModuleName, &AS, TRUE);

		PEPROCESS process;
		PsLookupProcessByProcessId((HANDLE)instructions->pid, &process);
		ULONG64 base_address64 = NULL;
		base_address64 = get_module_base_x64(process, ModuleName);
		instructions->base_address = base_address64;
		RtlFreeUnicodeString(&ModuleName);
	}

	if (instructions->write != FALSE)
	{
		if (instructions->address < 0x7FFFFFFFFFFF && instructions->address > 0)
		{
			PVOID kernel_buff = ExAllocatePool(NonPagedPool, instructions->size);

			if (!kernel_buff)
				return STATUS_UNSUCCESSFUL;

			if (!memcpy(kernel_buff, instructions->buffer_address, instructions->size))
				return STATUS_UNSUCCESSFUL;

			PEPROCESS process;
			PsLookupProcessByProcessId((HANDLE)instructions->pid, &process);
			write_kernel_memory((HANDLE)instructions->pid, instructions->address, kernel_buff, instructions->size);
			ExFreePool(kernel_buff);
		}
	}

	if (instructions->read != FALSE)
	{
		if (instructions->address < 0x7FFFFFFFFFFF && instructions->address > 0)
			read_kernel_memory((HANDLE)instructions->pid, instructions->address, instructions->output, instructions->size);
	}

	return STATUS_SUCCESS;
}