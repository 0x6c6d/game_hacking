// compile: cl .\4_Strings.cpp /I"C:\Dev\vcpkg\installed\x64-windows\include"
// execute: .\4_Strings.exe

/*
*  The UNICODE_STRING structure is a string descriptor - 
*  it describes a string, but does not necessarily own it.
*  Strings doesn't have to be NULL-terminated

typedef struct _UNICODE_STRING {  
	USHORT Length; // length in bytes (doesn't include NULL-terminator)
	USHORT MaximumLength; 
	PWCH Buffer; // pointer to the characters 
} UNICODE_STRING, *PUNICODE_STRING; 
typedef const UNICODE_STRING *PCUNICODE_STRING;
*/

#include <phnt_windows.h> 
#include <phnt.h>
#include <stdio.h>

int main() {
	// works but slightly inefficient: calculation of the string length is performed during runtime
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, L"Name");

	// more efficient: calculation of string length is performed during compilation
	UNICODE_STRING name2 = RTL_CONSTANT_STRING(L"Name2");

	// %.*s is a format specifier that takes two arguments:
	//		First: number of characters to print(int)
	//		Second : pointer to the string(wchar_t*)
	wprintf(L"%.*s, %.*s\n",
		(int)(name.Length / sizeof(WCHAR)), name.Buffer,
		(int)(name2.Length / sizeof(WCHAR)), name2.Buffer);

	return 0;
}