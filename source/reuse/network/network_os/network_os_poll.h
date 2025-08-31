#ifndef network_os_poll_h
#define network_os_poll_h

#include "../../base/base_include.h"
#include "../network_socket.h"
#include "../network_poll.h"

socket_polling * SocketPollingCreate(memory_arena * Arena, u32 Sockets);
i32 SocketPollingAdd(socket_polling * Instance, socket_handle Socket, i32 Index);
void SocketPollingRemove(socket_polling * Instance, i32 Index);
i32 SocketPollingPoll(socket_polling * Instance, i32 Timeout);
bool32 SocketPollingGet(socket_polling * Instance, i32 Index);

#endif