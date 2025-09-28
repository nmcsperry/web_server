#include "../../../base/base_include.h"
#include "../network_os_include.h"

#include "network_win32_internal.h"

void SocketInit()
{
	WSADATA WsaData;
	WORD WinSockVersion = MAKEWORD(2, 2);
	i32 Value = WSAStartup(WinSockVersion, &WsaData);
	if (Value != 0)
	{
		return; // error
	}
}

// ip address

ip_addr IPAddrGetIPv4Address(u8 A, u8 B, u8 C, u8 D)
{
	ip_addr Address = (ip_addr) { .IsValid = true };
	Address.AddrU8[0] = D;
	Address.AddrU8[1] = C;
	Address.AddrU8[2] = B;
	Address.AddrU8[3] = A;
	Address.AddrU32[1] = SwapByteOrderU32(0x0000ffff); // This means it's IPv4
	return Address;
}

ip_addr IPAddrGetIPv6Address(u16 A, u16 B, u16 C, u16 D, u16 E, u16 F, u16 G, u16 H)
{
	ip_addr Address = (ip_addr) { .IsValid = true };
	Address.AddrU16[0] = SwapByteOrderU16(H);
	Address.AddrU16[1] = SwapByteOrderU16(G);
	Address.AddrU16[2] = SwapByteOrderU16(F);
	Address.AddrU16[3] = SwapByteOrderU16(E);
	Address.AddrU16[4] = SwapByteOrderU16(D);
	Address.AddrU16[5] = SwapByteOrderU16(C);
	Address.AddrU16[6] = SwapByteOrderU16(B);
	Address.AddrU16[7] = SwapByteOrderU16(A);
	return Address;
}

// general

socket_handle SocketCreate(socket_type Type)
{
	win32_socket_handle Result = INVALID_SOCKET;

	switch (Type)
	{
	case SocketType_WebIPv4: {
		Result = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	} break;

	case SocketType_WebIPv6: {
		Result = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	} break;

	default: {
		return Win32SocketWrap(INVALID_SOCKET);
	} break;
	}

	return Win32SocketWrap(Result);
}

void SocketSetNonblocking(socket_handle Socket, bool32 Value)
{
	if (!Socket.IsValid)
	{
		return;
	}

	u_long NonBlocking = Value;
	int Result = ioctlsocket(Win32SocketUnwrap(Socket), FIONBIO, &NonBlocking);
}

void SocketSetReuseAddr(socket_handle Socket, bool32 Value)
{
	if (!Socket.IsValid)
	{
		return;
	}

	u8 ReuseOption = Value;
	setsockopt(Win32SocketUnwrap(Socket), SOL_SOCKET, SO_REUSEADDR, (const char *) &ReuseOption, sizeof(ReuseOption));
}

bool32 SocketHasData(socket_handle Socket)
{
	if (!Socket.IsValid)
	{
		return false;
	}

	return !!ioctlsocket(Win32SocketUnwrap(Socket), FIONREAD, 0);
}

bool32 SocketIsIPv6(socket_handle Socket)
{
	if (!Socket.IsValid)
	{
		return false;
	}

	WSAPROTOCOL_INFOA Result = { 0 };
	int ResultLength = sizeof(Result);
	getsockopt(Win32SocketUnwrap(Socket), SOL_SOCKET, SO_PROTOCOL_INFOA, (char *) & Result, &ResultLength);

	return Result.iAddressFamily == AF_INET6;
}

// server

bool32 SocketBind(socket_handle Socket, u16 Port)
{
	if (!Socket.IsValid)
	{
		return false;
	}

	if (SocketIsIPv6(Socket))
	{
		SOCKADDR_IN6 IPv6Address = { 0 };

		IPv6Address.sin6_family = AF_INET6;

		IPv6Address.sin6_port = SwapByteOrderU16(Port);
		IPv6Address.sin6_addr.u.Word[0] = 0;
		IPv6Address.sin6_addr.u.Word[1] = 0;
		IPv6Address.sin6_addr.u.Word[2] = 0;
		IPv6Address.sin6_addr.u.Word[3] = 0;
		IPv6Address.sin6_addr.u.Word[4] = 0;
		IPv6Address.sin6_addr.u.Word[5] = 0;
		IPv6Address.sin6_addr.u.Word[6] = 0;
		IPv6Address.sin6_addr.u.Word[7] = 0;

		bind(Win32SocketUnwrap(Socket), (void *) &IPv6Address, sizeof(IPv6Address));
	}
	else
	{
		SOCKADDR_IN IPv4Address = { 0 };

		IPv4Address.sin_family = AF_INET;

		IPv4Address.sin_port = SwapByteOrderU16(Port);
		IPv4Address.sin_addr.s_addr = 0;

		bind(Win32SocketUnwrap(Socket), (void *) &IPv4Address, sizeof(IPv4Address));
	}

	return true;
}

void SocketListen(socket_handle Socket, u32 Backlog)
{
	if (!Socket.IsValid)
	{
		return;
	}

	listen(Win32SocketUnwrap(Socket), Backlog);
}

bool32 SocketAccept(socket_handle OurSocket, socket_handle * ConnectionSocket, ip_addr * Address)
{
	if (!OurSocket.IsValid)
	{
		return false;
	}

	SOCKADDR_STORAGE AddressInfo = { 0 };
	u32 Size = sizeof(AddressInfo);

	win32_socket_handle NewSocket = WSAAccept(Win32SocketUnwrap(OurSocket), (void *) &AddressInfo, &Size, 0, 0);

	if (NewSocket == INVALID_SOCKET)
	{
		int Error = WSAGetLastError();
		if (Error == WSAEWOULDBLOCK)
		{
			return false; // not an error
		}

		return false; // error
	}

	if (AddressInfo.ss_family == AF_INET)
	{
		if (Address)
		{
			SOCKADDR_IN * IPv4AddressInfo = (SOCKADDR_IN *) &AddressInfo;
			Address->AddrU32[0] = IPv4AddressInfo->sin_addr.s_addr;
			Address->AddrU32[1] = SwapByteOrderU32(0x0000ffff); // This means it's IPv4
			Address->AddrU32[2] = 0;
			Address->AddrU32[3] = 0;
		}
	}
	else if (AddressInfo.ss_family == AF_INET6)
	{
		if (Address)
		{
			SOCKADDR_IN6 * IPv6AddressInfo = (SOCKADDR_IN6 *) &AddressInfo;
			Address->AddrU16[0] = IPv6AddressInfo->sin6_addr.u.Word[0];
			Address->AddrU16[1] = IPv6AddressInfo->sin6_addr.u.Word[1];
			Address->AddrU16[2] = IPv6AddressInfo->sin6_addr.u.Word[2];
			Address->AddrU16[3] = IPv6AddressInfo->sin6_addr.u.Word[3];
			Address->AddrU16[4] = IPv6AddressInfo->sin6_addr.u.Word[4];
			Address->AddrU16[5] = IPv6AddressInfo->sin6_addr.u.Word[5];
			Address->AddrU16[6] = IPv6AddressInfo->sin6_addr.u.Word[6];
			Address->AddrU16[7] = IPv6AddressInfo->sin6_addr.u.Word[7];
		}
	}
	else // error, sort of
	{
		closesocket(NewSocket);
		return false;
	}

	*ConnectionSocket = Win32SocketWrap(NewSocket);
	return true;
}

// client

bool32 SocketConnect(socket_handle Socket, ip_addr Address, u16 Port)
{
	if (!Address.IsValid || !Socket.IsValid)
	{
		return false;
	}

	if (IPAddrIsIPv6(Address))
	{
		if (!SocketIsIPv6(Socket))
		{
			return false;
		}

		SOCKADDR_IN6 IPv6Address = { 0 };
		IPv6Address.sin6_family = AF_INET6;
		IPv6Address.sin6_port = SwapByteOrderU16(Port);
		IPv6Address.sin6_addr.u.Word[0] = Address.AddrU16[0];
		IPv6Address.sin6_addr.u.Word[1] = Address.AddrU16[1];
		IPv6Address.sin6_addr.u.Word[2] = Address.AddrU16[2];
		IPv6Address.sin6_addr.u.Word[3] = Address.AddrU16[3];
		IPv6Address.sin6_addr.u.Word[4] = Address.AddrU16[4];
		IPv6Address.sin6_addr.u.Word[5] = Address.AddrU16[5];
		IPv6Address.sin6_addr.u.Word[6] = Address.AddrU16[6];
		IPv6Address.sin6_addr.u.Word[7] = Address.AddrU16[7];

		if (connect(Win32SocketUnwrap(Socket), (void *) &IPv6Address, sizeof(IPv6Address)))
		{
			return false;
		}
	}
	else
	{
		SOCKADDR_IN IPv4Address = { 0 };
		IPv4Address.sin_family = AF_INET;
		IPv4Address.sin_port = SwapByteOrderU16(Port);
		IPv4Address.sin_addr.s_addr = Address.AddrU32[0];

		if (connect(Win32SocketUnwrap(Socket), (void *) &IPv4Address, sizeof(IPv4Address)))
		{
			return false;
		}
	}

	return true;
}

// read and write

str8 SocketInputToPtr(socket_handle Socket, void * Memory, u32 MaxCount)
{
	if (!Socket.IsValid)
	{
		return Str8Empty();
	}

	u32 TotalBytesWritten = 0;
	i32 LastBytesWritten = 0;

	do {
		LastBytesWritten = recv(Win32SocketUnwrap(Socket), (u8 *) Memory + TotalBytesWritten, MaxCount - TotalBytesWritten, 0);
		if (LastBytesWritten > 0)
		{
			TotalBytesWritten += LastBytesWritten;
		}
	} while (LastBytesWritten > 0);

	return (str8) { .Data = Memory, .Count = TotalBytesWritten };
}

str8 SocketInputToBuffer(socket_handle Socket, memory_buffer * Buffer)
{
	str8 Written = SocketInputToPtr(Socket, BufferAt(Buffer), BufferLeft(Buffer));
	BufferPushNoWrite(Buffer, Written.Count);
	return Written;
}

str8 SocketInput(socket_handle Socket, memory_arena * Arena)
{
	memory_buffer * Buffer = ScratchBufferStart();

	BufferPushNoWrite(Buffer, SocketInputToPtr(Socket, BufferAt(Buffer), BufferLeft(Buffer)).Count);
	Str8WriteChar8(Buffer, '\0');

	return ScratchBufferEndStr8(Buffer, Arena);
}

void SocketClose(socket_handle Socket)
{
	if (!Socket.IsValid)
	{
		return;
	}

	closesocket(Win32SocketUnwrap(Socket));
}

void SocketOutput(socket_handle Socket, str8 String)
{
	if (!Socket.IsValid)
	{
		return;
	}

	send(Win32SocketUnwrap(Socket), String.Data, String.Count, 0);
}