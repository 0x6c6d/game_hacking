#include <windows.h> 
#include <stdio.h> 
#include "..\..\km\src\Common.h"

int Error(const char* message) { 
	printf("%s (error=%u)\n", message, GetLastError());
	return 1; 
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: ThreadPriority <threadid> <priority>\n");
	}

	int tid = atoi(argv[1]);
	int priority = atoi(argv[2]);

	// open handle to driver
	//		should reach the driver in its IRP_MJ_CREATE dispatch routine
	HANDLE hDevice = CreateFile(L"\\\\.\\ThreadPriority", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) 
			return Error("Failed to open device");
}