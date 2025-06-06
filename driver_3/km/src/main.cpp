// installation & execution
// > sc create tp type= kernel binPath= C:\Test\km.sys
// > sc start tp
// check with WinObj > GLOBAL?? > tp

#include <ntifs.h> // already includes <ntddk.h>
#include "Common.h"

// prototypes
void CustomUnload(_In_ PDRIVER_OBJECT driver_obj);
NTSTATUS CustomCreateClose(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS CustomWrite(PDEVICE_OBJECT device_obj, PIRP irp);

// entry
extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING reg_path) {
	UNREFERENCED_PARAMETER(reg_path);

	// register custom unload function
	driver_obj->DriverUnload = CustomUnload;
	// add dispatch routines
	driver_obj->MajorFunction[IRP_MJ_CREATE] = CustomCreateClose;
	driver_obj->MajorFunction[IRP_MJ_CLOSE] = CustomCreateClose;
	driver_obj->MajorFunction[IRP_MJ_WRITE] = CustomWrite;

	// create device object
	UNICODE_STRING device_name = RTL_CONSTANT_STRING(L"\\Device\\tp");
	PDEVICE_OBJECT device_obj;
	NTSTATUS status = IoCreateDevice(driver_obj, 0, &device_name, FILE_DEVICE_UNKNOWN, 0, FALSE, &device_obj);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device (0x%08X)\n", status));
		return status;
	}

	// make it accessible for um via symbolic link
	UNICODE_STRING sym_link = RTL_CONSTANT_STRING(L"\\??\\tp");
	status = IoCreateSymbolicLink(&sym_link, &device_name);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create symbolic link (0x%08X)\n", status));
		IoDeleteDevice(device_obj);
		return status;
	}

	return STATUS_SUCCESS;
}

void CustomUnload(_In_ PDRIVER_OBJECT driver_obj) {
	// unload device object and symblic link in reverse order
	KdPrint(("ThreadPriority: unload driver\n"));

	UNICODE_STRING sym_link = RTL_CONSTANT_STRING(L"\\??\\tp"); 
	// delete symbolic link 
	IoDeleteSymbolicLink(&sym_link); 
	
	// delete device object 
	IoDeleteDevice(driver_obj->DeviceObject);
}

// pirp = pointer to I/O request packet
//		primary object where request information are stored
NTSTATUS CustomCreateClose(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	// propagates the irp back to its creator (e.g. I/O manager)
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

// using "WriteFile" for um/km communication (DeviceIoControl & ReadFile are other communication options)
NTSTATUS CustomWrite(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	auto status = STATUS_SUCCESS;
	ULONG_PTR information = 0; // track used bytes

	auto irp_sp = IoGetCurrentIrpStackLocation(irp);
	do { // allows do/while(false) loop to use the break keyword
		if (irp_sp->Parameters.Write.Length < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		auto data = static_cast<ThreadData*>(irp->UserBuffer);
		// short circuit evaluation: check for nullptr first & skip other checks if nullptr
		if (data == nullptr || data->Priority < 1 || data->Priority > 31) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		
		// get a handle to our thread to call KeSetPriorityThread()
		// the thread cannot die after optaining its pointer because the lookup function increments 
		// the reference count on the kernel thread object (cannot die until we explicitly
		// decrement the reference count)
		PETHREAD thread; 
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &thread); 
		if (!NT_SUCCESS(status)) 
			break;

		// set the thread priority
		auto old_prio = KeSetPriorityThread(thread, data->Priority); 
		KdPrint(("Priority change for thread %u from %d to %d succeeded!\n", 
			data->ThreadId, old_prio, data->Priority));	
		UNREFERENCED_PARAMETER(old_prio);

		// decrement the kernel thread objects reference count so it can be terminated
		ObDereferenceObject(thread);

		// report to the client that we used the provided buffer
		information = sizeof(data);
	} while (false);

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = information;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return status;
}