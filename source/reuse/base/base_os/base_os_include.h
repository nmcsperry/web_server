#ifndef base_os_include_h
#define base_os_include_h

#if defined OS_WINDOWS
	#define _WINSOCKAPI_
	#include <windows.h>
#elif defined OS_MAC
	#include <stdlib.h>
	#include <string.h>
	#include <sys/mman.h>
#elif defined OS_WASM
#endif

#include "base_os_memory.h"
#include "base_os_time.h"

#endif