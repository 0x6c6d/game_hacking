#include <ntddk.h>
#include "Common.h"

// prototypes
void CustomUnload(PDRIVER_OBJECT driver_obj);
NTSTATUS CustomCreateClose(PDEVICE_OBJECT device_obj, PIRP irp);
NTSTATUS CustomWrite(PDEVICE_OBJECT device_obj, PIRP irp);

// entry
extern "C" NTSTATUS
DriverEntry(PDRIVER_OBJECT driver_obj, PUNICODE_STRING reg_path) {
	driver_obj->DriverUnload = CustomUnload;

	// add dispatch routines
	driver_obj->MajorFunction[IRP_MJ_CREATE] = CustomCreateClose;
	driver_obj->MajorFunction[IRP_MJ_CLOSE] = CustomCreateClose;
	driver_obj->MajorFunction[IRP_MJ_WRITE] = CustomWrite;

	return STATUS_SUCCESS;
}

void CustomUnload(PDRIVER_OBJECT driver_obj) {

}

// pirp = pointer to I/O request packet
//		primary object where request information are stored
NTSTATUS CustomCreateClose(PDEVICE_OBJECT device_obj, PIRP irp) {
}

// using "WriteFile" for um/km communication (DeviceIoControl & ReadFile are other communication options)
NTSTATUS CustomWrite(PDEVICE_OBJECT device_obj, PIRP irp) {
	
}