// compile: cl 2_DynamicLinkingNtdll.cpp
// execute: .\2_DynamicLinkingNtdll.exe

#include <Windows.h>
#include <stdio.h>

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,
} SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_BASIC_INFORMATION {
	ULONG Reserved;
	ULONG TimerResolution;
	ULONG PageSize;
	ULONG NumberOfPhysicalPages;
	ULONG LowestPhysicalPageNumber;
	ULONG HighestPhysicalPageNumber;
	ULONG AllocationGranularity;
	ULONG_PTR MinimumUserModeAddress;
	ULONG_PTR MaximumUserModeAddress;
	ULONG_PTR ActiveProcessorsAffinityMask;
	CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, * PSYSTEM_BASIC_INFORMATION;

typedef NTSTATUS(NTAPI* PNtQuerySystemInformation)(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength);

int main() {
	// Instead of linking against NtDll.lib at compile time, we load the API dynamically.
	// This avoids potential runtime crashes on systems where the target API is not supported.

	// no extern "C" needed: GetProcAddress always assumes an unmangled function name
	// GetModuleHandleW is used to retrieve the handle of ntdll.dll.
	//		This always succeeds because ntdll.dll is mapped into every user-mode process by the OS.
	// GetProcAddress retrieves the address of NtQuerySystemInformation from ntdll.dll.
	//		Easy error handling because if the function is not available it returns NULL
	auto NtQuerySystemInformation = (PNtQuerySystemInformation)GetProcAddress(
		GetModuleHandleW(L"ntdll"), "NtQuerySystemInformation");

	if (NtQuerySystemInformation) {
		SYSTEM_BASIC_INFORMATION sysInfo;
		NTSTATUS status = NtQuerySystemInformation(SystemBasicInformation, &sysInfo, sizeof(sysInfo), nullptr);
		if (status == 0) {
			printf("Page size: %u bytes\n", sysInfo.PageSize);
			printf("Processors: %u\n", (ULONG)sysInfo.NumberOfProcessors);
			printf("Physical pages: %u\n", sysInfo.NumberOfPhysicalPages);
			printf("Lowest  Physical page: %u\n", sysInfo.LowestPhysicalPageNumber);
			printf("Highest Physical page: %u\n", sysInfo.HighestPhysicalPageNumber);
		}
		else {
			printf("Error calling NtQuerySystemInformation (0x%X)\n", status);
		}
	}
	return 0;
}