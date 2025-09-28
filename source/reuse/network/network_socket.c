#include "network_os/network_os_include.h"

socket_handle SocketCreateServer(u16 Port, u32 Backlog)
{
	socket_handle Socket = SocketCreate(SocketType_WebIPv4);
	SocketSetNonblocking(Socket, true);
	SocketSetReuseAddr(Socket, true);
	SocketBind(Socket, Port);
	SocketListen(Socket, Backlog);
	return Socket;
}

socket_handle SocketCreateClient(u16 Port, ip_addr Address)
{
	socket_handle Socket = SocketCreate(SocketType_WebIPv4);
	SocketSetNonblocking(Socket, true);
	SocketSetReuseAddr(Socket, true);
	SocketConnect(Socket, Address, Port);

	return Socket;
}

socket_handle SocketGetInvalid()
{
	return (socket_handle) { .IsValid = false };
}

ip_addr IPAddrGetInvalid()
{
	return (ip_addr) { .IsValid = false };
}

ip_addr IPAddrGetIPv4Localhost()
{
	return IPAddrGetIPv4Address(127, 0, 0, 1);
}

ip_addr IPAddrGetIPv6Localhost()
{
	return IPAddrGetIPv6Address(0, 0, 0, 0, 0, 0, 0, 1);
}

bool32 IPAddrIsIPv6(ip_addr Address)
{
	if (!Address.IsValid) return false;
	return Address.AddrU32[3] != 0 || Address.AddrU32[2] != 0 || Address.AddrU32[1] != SwapByteOrderU32(0x0000ffff);
}

ip_addr IPAddrFromStr8(str8 String)
{
	if (Str8Find(String, Str8Lit("."), MatchFlag_Normal) != -1) // IPv4 address
	{
		u8 Segments[4] = { 0 };

		for (i32 I = 0; I < 4; I++)
		{
			str8_split Split = Str8CutFind(String, Str8Lit("."));
			if (Split.First.Count == 0) { return IPAddrGetInvalid(); }

			String = Split.Second;

			str8 Extra = { 0 };
			Segments[I] = IntFromStr8(Split.First, &Extra);
			if (Extra.Count) { return IPAddrGetInvalid(); }
		}

		return IPAddrGetIPv4Address(Segments[0], Segments[1], Segments[2], Segments[3]);
	}
	else if (Str8Find(String, Str8Lit(":"), MatchFlag_Normal) != -1) // IPv6 address
	{
		bool32 ContainsFlexibleSegment = Str8Find(String, Str8Lit("::"), MatchFlag_Normal) != -1;
		str8_split Parts = Str8CutFind(String, Str8Lit("::"));
		
		u16 Segments[8] = { 0 };

		u8 FirstPartCursor = 0;
		while (Parts.First.Count)
		{
			if (FirstPartCursor >= 8)
			{
				return IPAddrGetInvalid();
			}

			str8_split Split = Str8CutFind(Parts.First, Str8Lit(":"));
			if (Split.First.Count == 0)
			{
				return IPAddrGetInvalid();
			}

			Parts.First = Split.Second;

			str8 Extra = { 0 };
			Segments[FirstPartCursor] = IntFromHexStr8(Split.First, &Extra);
			if (Extra.Count)
			{
				return IPAddrGetInvalid();
			}

			FirstPartCursor++;
		}

		if (ContainsFlexibleSegment)
		{
			u8 SecondPartCursor = 0;
			while (Parts.Second.Count)
			{
				if (FirstPartCursor >= 8)
				{
					return IPAddrGetInvalid();
				}

				str8_split Split = Str8CutFind(Parts.Second, Str8Lit(":"));
				if (Split.First.Count == 0)
				{
					return IPAddrGetInvalid();
				}

				Parts.Second = Split.Second;

				str8 Extra = { 0 };
				Segments[7 - SecondPartCursor] = IntFromHexStr8(Split.First, &Extra);
				if (Extra.Count)
				{
					return IPAddrGetInvalid();
				}

				SecondPartCursor++;
			}

			if (SecondPartCursor + FirstPartCursor > 8)
			{
				return IPAddrGetInvalid();
			}
		}
		else if (FirstPartCursor == 8)
		{
			return IPAddrGetInvalid();
		}

		return IPAddrGetIPv6Address(Segments[0], Segments[1], Segments[2], Segments[3], Segments[4], Segments[5], Segments[6], Segments[7]);
	}
	else
	{
		return IPAddrGetInvalid();
	}
}

ip_addr IPAddrFromCStr(char * String)
{
	return IPAddrFromStr8(Str8FromCStr(String));
}

str8 Str8FromIPAddr(memory_arena * Arena, ip_addr Address)
{
	if (!Address.IsValid) return Str8Lit("Invalid.");

	if (IPAddrIsIPv6(Address))
	{
		memory_buffer * Buffer = ScratchBufferStart();

		for (i32 I = 0; I < 8; I++)
		{
			Str8WriteHexDigits(Buffer, SwapByteOrderU16(Address.AddrU16[I]), 4);
			if (I != 7) { Str8WriteChar8(Buffer, ':'); }
		}

		return ScratchBufferEndStr8(Buffer, Arena);
	}
	else
	{
		memory_buffer * Buffer = ScratchBufferStart();

		for (i32 I = 0; I < 4; I++)
		{
			Str8WriteInt(Buffer, Address.AddrU8[I]);
			if (I != 3) { Str8WriteChar8(Buffer, '.'); }
		}

		return ScratchBufferEndStr8(Buffer, Arena);
	}
}

void SocketOutputFmt(socket_handle Socket, const char * FormatCStr, ...)
{
	memory_buffer * Buffer = ScratchBufferStart();

	va_list FormatArguments;
	va_start(FormatArguments, FormatCStr);

	Str8WriteFmtCore(Buffer, Str8FromCStr((char *) FormatCStr), FormatArguments);

	va_end(FormatArguments);

	SocketOutput(Socket, Str8FromBuffer(Buffer));

	ScratchBufferRelease(Buffer);
}