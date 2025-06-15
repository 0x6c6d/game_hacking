#include <iostream>
#include "driver.h"

int main()
{
	printf("[i] program started\n");
	
	while (!globals::process_id) {
		globals::process_id = driver::get_process_id("chrome.exe");
		Sleep(1000);
	}
	printf("[i] found process id: %p\n", globals::process_id);

	while (!globals::module_base) {
		globals::module_base = driver::get_module_base(globals::process_id, "chrome.exe");
		Sleep(1000);
	}
	printf("[i] found module base: %p\n", (void*)globals::module_base);

	while (!globals::peb_address) {
		globals::peb_address = driver::get_peb_address(globals::process_id);
		Sleep(1000);
	}
	printf("[i] found peb address: %p\n", (void*)globals::peb_address);
		
	std::cin.get();
	return 0;
}