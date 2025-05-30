#include <ntifs.h>

extern "C" {
	// makes driver compatible with kdmapper
	NTKERNELAPI NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName,
		PDRIVER_INITIALIZE InitializationFunction);

	// read & write process memory inside our driver
	NTKERNELAPI NTSTATUS MmCopyVirtualMemory(PEPROCESS SourceProcess, PVOID SourceAddress,
		PEPROCESS TargetProcess, PVOID TargetAddress,
		SIZE_T BufferSize, KPROCESSOR_MODE PreviousMode,
		PSIZE_T ReturnSize);
}

void debug_print(PCSTR text) {
#ifndef DEBUG
	UNREFERENCED_PARAMETER(text);
#endif // !DEBUG

	KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, text));

}

namespace driver {
	namespace code {
		// IOCTL codes
	}
}

NTSTATUS DriverEntry() {
	debug_print("[+] DriverEntry triggered");

	return STATUS_SUCCESS;
}