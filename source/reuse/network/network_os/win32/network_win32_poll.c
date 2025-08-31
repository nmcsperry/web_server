#include "../../../base/base_include.h"
#include "../network_os_include.h"
#include "../../network_poll.h"

#include "network_win32_internal.h"

#define WSAPOLLFD_IGNORE -1

struct socket_polling
{
	WSAPOLLFD * PollStructs;
	u32 SocketCount;
};

socket_polling * SocketPollingCreate(memory_arena * Arena, u32 Sockets)
{
	WSAPOLLFD * PollStructs = ArenaPushBytes((Arena), sizeof(WSAPOLLFD) * (Sockets), _Alignof(WSAPOLLFD));

	for (u32 I = 0; I < Sockets; I++)
	{
		PollStructs[I].fd = WSAPOLLFD_IGNORE;
		PollStructs[I].events = POLLRDNORM;
	}

	socket_polling * Result = ArenaPush(Arena, socket_polling);
	Result->PollStructs = PollStructs;
	Result->SocketCount = Sockets;

	return Result;
}

i32 SocketPollingAdd(socket_polling * Instance, socket_handle Socket, i32 Index)
{
	if (!Socket.IsValid)
	{
		Instance->PollStructs[Index].fd = WSAPOLLFD_IGNORE;
		Instance->PollStructs[Index].events = POLLRDNORM;

		return -1;
	}

	Instance->PollStructs[Index].fd = Win32SocketUnwrap(Socket);
	Instance->PollStructs[Index].events = POLLRDNORM;

	return Index;
}

void SocketPollingRemove(socket_polling * Instance, i32 Index)
{
	Instance->PollStructs[Index].fd = WSAPOLLFD_IGNORE;
	Instance->PollStructs[Index].events = POLLRDNORM;
}

i32 SocketPollingPoll(socket_polling * Instance, i32 Timeout)
{
	return WSAPoll(Instance->PollStructs, Instance->SocketCount, Timeout);
}

bool32 SocketPollingGet(socket_polling * Instance, i32 Index)
{
	if (Index < 0 || (((u32) Index) >= Instance->SocketCount)) return false;

	return !!(Instance->PollStructs[Index].revents & POLLRDNORM);
}

#undef WSAPOLLFD_IGNORE