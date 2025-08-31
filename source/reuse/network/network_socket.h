#ifndef network_socket_h
#define network_socket_h

typedef struct socket_handle
{
	bool8 IsValid;
	union {
		u64 OSDataU64;
		i64 OSDataI64;
		u32 OSDataU32[2];
		i32 OSDataI32[2];
		void * OSDataPointer;
	};
} socket_handle;

typedef enum socket_type
{
	SocketType_WebIPv4,
	SocketType_WebIPv6,
	SocketType_COUNT
} socket_type;

// IPv4 addresses are 32 bits. IPv6 addresses are 128 bits.
// This IPv6 range is used to store an IPv4 address: 0x00000000000000000000ffff________

// in network byte order
typedef struct ip_addr
{
	union {
		u64 AddrU64[2];
		u32 AddrU32[4];
		u16 AddrU16[8];
		u8 AddrU8[16];
	};
	bool32 IsValid;
} ip_addr;

socket_handle SocketCreateServer(u16 Port, u32 Backlog);
socket_handle SocketCreateClient(u16 Port, ip_addr Address);
socket_handle SocketGetInvalid();

ip_addr IPAddrGetIPv4Localhost(); // 127.0.0.1
ip_addr IPAddrGetIPv6Localhost(); // ::1
bool32 IPAddrIsIPv6(ip_addr Address);

ip_addr IPAddrFromStr8(str8 String);
ip_addr IPAddrFromCStr(char * String);
str8 Str8FromIPAddr(memory_arena * Arena, ip_addr Address);

void SocketOutputFmt(socket_handle Socket, const char * FormatCStr, ...);

#endif