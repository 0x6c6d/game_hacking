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
	// IOCTL codes
		// input / output control > common way for drivers to communicate with user mode applications
	namespace codes {
		// used to setup the driver
		constexpr ULONG attach =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x696, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// read process memory
		constexpr ULONG read =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x697, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

		// write process memory
		constexpr ULONG write =
			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x698, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	} // namespace codes

	// shares between user mode & kernel mode
	struct Request {
		HANDLE process_id;
		PVOID target;
		PVOID buffer;
		SIZE_T size;
		SIZE_T return_size;
	};

	NTSTATUS create(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	NTSTATUS close(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return irp->IoStatus.Status;
	}

	// when sending data from um to km via DeviceIoControl it goes through this function
	//		irp holds data that will be send from um
	NTSTATUS device_control(PDEVICE_OBJECT device_object, PIRP irp) {
		UNREFERENCED_PARAMETER(device_object);

		debug_print("[+] device control called\n");

		NTSTATUS status = STATUS_UNSUCCESSFUL;

		// needed to determine which code was passed through
		PIO_STACK_LOCATION stack_irp = IoGetCurrentIrpStackLocation(irp);

		// access the request object sent from um
		auto request = reinterpret_cast<Request*>(irp->AssociatedIrp.SystemBuffer);

		if (stack_irp == nullptr || request == nullptr) {
			IoCompleteRequest(irp, IO_NO_INCREMENT);
			return status;
		}

		// target process we want to access
		//		static means the value persists beyond the lifetime of the function
		static PEPROCESS target_process = nullptr;

		const ULONG control_code = stack_irp->Parameters.DeviceIoControl.IoControlCode;
		switch (control_code) {
			// populate target_process with a process (get a handle to the process)
			case codes::attach:
				status = PsLookupProcessByProcessId(request->process_id, &target_process);
				break;
			case codes::read:
				if (target_process != nullptr) {
					status = MmCopyVirtualMemory(target_process, request->target, PsGetCurrentProcess(), 
						request->buffer, request->size, KernelMode, &request->return_size);
				}
				break;
			case codes::write:
				if (target_process != nullptr) {
					status = MmCopyVirtualMemory(PsGetCurrentProcess(), request->buffer, target_process, 
						request->target, request->size, KernelMode, &request->return_size);
				}
				break;
			default:
				break;
		}

		irp->IoStatus.Status = status;
		irp->IoStatus.Information = sizeof(Request);

		IoCompleteRequest(irp, IO_NO_INCREMENT);

		return status;
	}
} // namespace driver


// "real" entry point
NTSTATUS driver_main(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
	UNREFERENCED_PARAMETER(registry_path);

	UNICODE_STRING device_name = {};
	RtlInitUnicodeString(&device_name, L" \\Device\\CS_1");

	// create driver device object
	PDEVICE_OBJECT device_object = nullptr;
	NTSTATUS status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN, FALSE, &device_object);

	if (status != STATUS_SUCCESS) {
		debug_print("[-] failed to create driver device\n");
		return status;
	}

	debug_print("[+] driver device successfully created\n");

	UNICODE_STRING symbolic_link = { };
	RtlInitUnicodeString(&symbolic_link, L"\\DosDevices\\CS_1");

	status = IoCreateSymbolicLink(&symbolic_link, &device_name);
	if (status != STATUS_SUCCESS) {
		debug_print("[-] failed to eastablish symbolic link\n");
		return status;
	}

	debug_print("[+] driver symbolic link successfully established\n");

	// allows to send small amounts of data between um/km
	SetFlag(device_object->Flags, DO_BUFFERED_IO);

	// set the driver handlers to custom functions
	//		inside the driver object is an array of functions (MajorFunction)
	//		when events happen the function that are saved in the MajorFunction array are called
	driver_object->MajorFunction[IRP_MJ_CREATE] = driver::create;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = driver::close;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = driver::device_control;

	// initialize the device
	ClearFlag(device_object->Flags, DO_DEVICE_INITIALIZING);

	debug_print("[+] driver initialized successfully\n");

	return STATUS_SUCCESS;
}

/*
* normaly a ref to a driver object is passed to the entry point
* because kdmapper is used they would be null
*	kdmapper manually maps the driver so the os doesn't know about the driver & cannot provide a driver object
*	the driver object needs to be manually created (springboard from DriverEntry() into a new entry point)
*
NTSTATUS DriverEntry(
  _In_ PDRIVER_OBJECT  DriverObject,
  _In_ PUNICODE_STRING RegistryPath
);
*/
NTSTATUS DriverEntry() {
	debug_print("[+] DriverEntry triggered\n");

	UNICODE_STRING driver_name = {};
	RtlInitUnicodeString(&driver_name, L"\\Driver\\CS_1"); // places the string (L"...") in the unicode_string structure

	return IoCreateDriver(&driver_name, &driver_main);
}