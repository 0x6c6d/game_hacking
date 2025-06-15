#include <iostream>
#include "driver.h"

int main()
{
	printf("[i] program started\n");
	
	while (!global_vars::process_id) {
		global_vars::process_id = driver::get_process_id("chrome.exe");
		Sleep(1000);
	}
	printf("[i] found process id: %p\n", global_vars::process_id);
		

	std::cin.get();
	return 0;
}