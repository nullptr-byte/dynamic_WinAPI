#include "DynWAPI.h"
#include "signatureFunc.h"

#define KERNEL32 L"kernel32.dll"
#define USER32   L"user32.dll"

int main()
{
	// Modules arrays
	MODULE_INF arr[] = {
		{KERNEL32,0},
		{USER32,0}
	};

	
	// Init
	auto status = dynWAPI::init(arr, COUNT_OF(arr));
	if (!DWAPI_SUCCESS(status))
		return 0;

	// Example using
	DWAPI(KERNEL32, Sleep)(10000);
	auto tickCount = DWAPI(KERNEL32, GetTickCount)();
	
	return 1;
}