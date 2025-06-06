// usages (after installing driver)
// > um <threadid> <priority>

#include <windows.h> 
#include <stdio.h> 
#include "..\..\km\src\Common.h"

int Error(const char* message) { 
	printf("%s (error=%u)\n", message, GetLastError());
	return 1; 
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: um <threadid> <priority>\n");
	}

	int tid = atoi(argv[1]);
	int priority = atoi(argv[2]);

	// open handle to driver
	//		should reach the driver in its IRP_MJ_CREATE dispatch routine
	HANDLE hDevice = CreateFile(L"\\\\.\\tp", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) 
			return Error("Failed to open device");

	ThreadData data;
	data.ThreadId = tid;
	data.Priority = priority;

	DWORD returned; 
	// invokes the IRP_MJ_WRITE major function routine
	BOOL success = WriteFile(hDevice, &data, sizeof(data), &returned, nullptr); 
	if (!success) 
		return Error("Priority change failed!"); 
	
	printf("Priority change succeeded!\n"); 
	CloseHandle(hDevice);
}