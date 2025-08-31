#include "../../../base/base_include.h"
#include "../../io_include.h"

/*bitmap LoadBitmapFromResource(memory_arena * Memory, char * ResourceName) {
	HRSRC ResourceHandle = FindResourceA(0, ResourceName, RT_RCDATA);
	HGLOBAL ResourceDataHandle = LoadResource(0, ResourceHandle);

	str8 Data = { 0 };
	Data.Data = LockResource(ResourceDataHandle);
	Data.Count = SizeofResource(0, ResourceHandle);

	bitmap Result = ParseBMP(Memory, Data);

	FreeResource(ResourceDataHandle);

	return Result;
}*/
