#include "base_include.h"

#ifndef OVERRIDE_BASE_ENTRY

// todo: move this into the base_os folder?

int main(int ArgCount, char ** Args)
{
	EntryHook();
	return 0;
}

#endif