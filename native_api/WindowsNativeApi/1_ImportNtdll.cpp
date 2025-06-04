// compile: cl 1_ImportNtdll.cpp
// execute: .\1_ImportNtdll.exe

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

// extern "C": force the compiler to treat the function as a C function
//		- C++ supports function overloading: the compiler performs name mangling to create unique names for each overloaded function
//		- C doesnt support overloading & NtQuerySystemInformation was compiled with C
//		- C: extern "C" tells the compiler not to mangle to code
extern "C" NTSTATUS NTAPI NtQuerySystemInformation(
	_In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Out_writes_bytes_opt_(SystemInformationLength) PVOID SystemInformation,
	_In_ ULONG SystemInformationLength,
	_Out_opt_ PULONG ReturnLength);

#pragma comment(lib, "ntdll")

int main() {
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
	return 0;
}