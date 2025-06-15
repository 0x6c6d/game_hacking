#include "imports.h"
#include "memory.h"
#include "hook.h"

NTSTATUS hook_handler(PVOID called_param)
{
	COPY_MEMORY* m = (COPY_MEMORY*)called_param;

	if (m->get_pid) {
		m->pid = memory::get_process_id(m->process_name);
	}
	else if (m->base) {
		PEPROCESS process;
		if (NT_SUCCESS(PsLookupProcessByProcessId(m->pid, &process))) {
			m->buffer = (void*)memory::get_module_base_x64(process);
		}
	}
	else if (m->peb) {
		PEPROCESS process;
		if (NT_SUCCESS(PsLookupProcessByProcessId(m->pid, &process))) {
			m->buffer = (void*)PsGetProcessPeb(process);
		}
	}
	else if (m->read) {
		memory::read_kernel_memory(m->pid, (PVOID)m->address, m->buffer, m->size);
	}
	else if (m->write) {
		memory::write_kernel_memory(m->pid, m->buffer, (PVOID)m->address, m->size);
	}

	return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);

	KdPrint(0, 0, "[i] DriverEntry: loaded\n");
	if (hook::call_kernel_function(&hook_handler)) {
		KdPrint(0, 0, "[i] Driver: function hooked - NtOpenCompositionSurfaceSectionInfo");
	}
	else {
		KdPrint(0, 0, "[e] Driver: failed to hook function.\n");
	}

	return STATUS_SUCCESS;
}