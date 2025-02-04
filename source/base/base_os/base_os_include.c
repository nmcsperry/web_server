#if defined OS_WINDOWS
#include "win32/base_win32_memory.c"
#include "win32/base_win32_time.c"
#elif defined OS_MAC
#include "macos/base_macos_memory.c"
#elif defined OS_WASM
#include "wasm/base_wasm_memory.c"
#endif