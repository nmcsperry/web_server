#if defined OS_WINDOWS
	#include "win32/io_win32_file.c"
	#include "win32/io_win32_basic.c"
#elif defined OS_MAC
	#include "macos/io_macos_file.c"
#elif defined OS_WASM
	#include "dummy/io_dummy_file.c"
#endif