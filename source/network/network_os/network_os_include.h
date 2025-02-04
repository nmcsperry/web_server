#ifndef network_os_include_h
#define network_os_include_h

#if defined OS_WINDOWS

	#pragma comment(lib, "Ws2_32.lib")

	#define _WINSOCKAPI_
	#include <windows.h>
	#include <WinSock2.h>
	#include <ws2tcpip.h>
#elif defined OS_MAC
#endif

#include "network_os_socket.h"
#include "network_os_poll.h"

#endif