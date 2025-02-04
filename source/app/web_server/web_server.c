#include "../../base/base_include.h"
#include "../../network/network_include.h"
#include "../../io/io_include.h"
#include "web_server_include.h"

void EntryHook()
{
	SocketInit();

	socket_handle Socket = SocketCreateServer(80, 50);

	http_server Server = ServerInit(Socket);

	while (true)
	{
		ServerLoop(&Server);

		for (i32 I = 0; I < HTTP_REQUEST_COUNT; I++)
		{
			http_request_slot * Request = &Server.Requests[I];

			if (Request->Value.Valid)
			{
				memory_arena * Arena = Request->Arena;
				http_connection_slot * Connection = &Server.Connections[Request->Value.ConnectionIndex];

				if (Connection->Value.Valid) {

					str8 Body = Str8Fmt(Arena, "<html><body><p>Hello, this is the website.</p>"
						"<p>You are asking about %{str8}.</p>"
						"</body></html>", Request->Value.Path);

					SocketOutputFmt(Connection->Value.Socket, "HTTP/1.1 200 OK\r\nConnection: Keep-Alive\r\nContent-Length: %{u32}\r\n\r\n%{str8}",
						Body.Count, Body);

					Connection->Value.ResponsesSent++;
				}
			}
		}

		FlushRequests(&Server);
	}
}