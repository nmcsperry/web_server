#include "network_win32_internal.h"

socket_handle Win32SocketWrap(win32_socket_handle WinHandle)
{
	if (WinHandle == INVALID_SOCKET)
	{
		return (socket_handle) { .OSDataU64 = INVALID_SOCKET, .IsValid = false };
	}

	return (socket_handle) { .OSDataU64 = WinHandle, .IsValid = true };
}

win32_socket_handle Win32SocketUnwrap(socket_handle FileHandle)
{
	if (!FileHandle.IsValid)
	{
		return INVALID_SOCKET;
	}
	return FileHandle.OSDataU64;
}