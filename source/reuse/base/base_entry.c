#include "base_include.h"

#if !defined OVERRIDE_BASE_ENTRY && !(OS_WASM)

// todo: move this into the base_os folder?

int main(int ArgCount, char ** Args)
{
	EntryHook();
	return 0;
}

#endif