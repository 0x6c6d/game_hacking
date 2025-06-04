// install vcpkg (use PS)
//		> create C:\Dev\ & go into it
//		> git clone https://github.com/microsoft/vcpkg.git
//		> cd vcpkg
//		> .\bootstrap-vcpkg.bat
//		# add "C:\Dev\vcpkg" to env variables paths
//		> vcpkg integrate install
// install phnt
//		vcpkg install phnt

// compile: cl .\3_PhntWindowsH.cpp /I"C:\Dev\vcpkg\installed\x64-windows\include"
// execute: .\3_PhntWindowsH.exe

#include <phnt_windows.h>
#include <phnt.h>
#include <stdio.h>

#pragma comment(lib, "ntdll")

int main() {
	SYSTEM_BASIC_INFORMATION sysInfo;
	NTSTATUS status = NtQuerySystemInformation(SystemBasicInformation, &sysInfo, sizeof(sysInfo), nullptr);
	if (status == STATUS_SUCCESS) {
		//
		// call succeeded
		//
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