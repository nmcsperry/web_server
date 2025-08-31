#if defined OS_WINDOWS
#include "win32/network_win32_internal.c"
#include "win32/network_win32_socket.c"
#include "win32/network_win32_poll.c"
#elif defined OS_MAC
#include "macos/network_macos_socket.c"
#endif