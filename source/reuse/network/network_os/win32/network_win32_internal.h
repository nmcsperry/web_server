#ifndef network_win32_internal_h
#define network_win32_internal_h

#include "../../../base/base_include.h"
#include "../network_os_include.h"

typedef SOCKET win32_socket_handle;

socket_handle Win32SocketWrap(win32_socket_handle WinHandle);
win32_socket_handle Win32SocketUnwrap(socket_handle FileHandle);

#endif