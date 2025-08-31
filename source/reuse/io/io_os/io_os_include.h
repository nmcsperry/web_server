#ifndef io_os_include_h
#define io_os_include_h

#if defined OS_WINDOWS
	#define _WINSOCKAPI_
	#include <windows.h>
#elif defined OS_MAC
	#include <unistd.h>
	#include <fcntl.h>
#elif defined OS_WASM
#endif

#include "io_os_file.h"
#include "io_os_basic.h"

#endif