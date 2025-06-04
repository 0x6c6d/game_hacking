// possible installation via Sc.exe (Service Control) -> needs to be done in CMD not PS
//		> sc create test_driver_1 type= kernel binPath= c:\...\build\km.sys
//		on 64 bit systems drivers need to be signed > starting it will fail
//			system need to be put in test signing mode (Secure Boot needs to be off)
//				> bcdedit /set testsigning on

#include <ntddk.h>

// driver can have an unload routine that is automatically called when the driver unloads from memory
void SampleUnload(_In_ PDRIVER_OBJECT driver_obj) {
	UNREFERENCED_PARAMETER(driver_obj);

	// KdPrint only outputs stuff in debug mode (saves ressources in release mode)
	//		a wrapper around DdgPrint();
	KdPrint(("Driver unloaded\n"));
}

// main entry point
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	// set unload function
	driver_obj->DriverUnload = SampleUnload;

	KdPrint(("Driver init success\n"));

	return STATUS_SUCCESS;
}
