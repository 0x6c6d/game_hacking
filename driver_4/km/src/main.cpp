#include "hook.h"

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING reg_path) {
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(reg_path);

	// the overwritten function will redirect to the hook_handler function
	nullhook::call_kernel_function(&nullhook::hook_handler);

	return STATUS_SUCCESS;
}