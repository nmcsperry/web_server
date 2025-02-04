#ifndef network_os_socket_h
#define network_os_socket_h

#include "../../base/base_include.h"
#include "../network_socket.h"

void SocketInit();

// ip address (I'm putting these in the platform specific area because byte order)
ip_addr IPAddrGetIPv4Address(u8 A, u8 B, u8 C, u8 D);
ip_addr IPAddrGetIPv6Address(u16 A, u16 B, u16 C, u16 D, u16 E, u16 F, u16 G, u16 H);

// general
socket_handle SocketCreate(socket_type Type);
void SocketSetNonblocking(socket_handle Socket, bool32 Value);
void SocketSetReuseAddr(socket_handle Socket, bool32 Value);
bool32 SocketHasData(socket_handle Socket);
bool32 SocketIsIPv6(socket_handle Socket);

// server
bool32 SocketBind(socket_handle Socket, u16 Port);
void SocketListen(socket_handle Socket, u32 Backlog);
bool32 SocketAccept(socket_handle OurSocket, socket_handle * ConnectionSocket, ip_addr * Address);

// client
bool32 SocketConnect(socket_handle Socket, ip_addr Address, u16 Port);

// read and write
str8 SocketInputToPtr(socket_handle Socket, void * Memory, u32 MaxCount);
str8 SocketInputToBuffer(socket_handle Socket, memory_buffer * Buffer);
str8 SocketInput(socket_handle Socket, memory_arena * Arena);

void SocketOutput(socket_handle Socket, str8 String);

void SocketClose(socket_handle Socket);

#endif