#include "../../base/base_include.h"
#include "../../network/network_include.h"
#include "../../io/io_include.h"
#include "web_server_include.h"

void EntryHook()
{
	buffer_pool BufferPool = BufferPoolInit();
	db_context Context = (db_context) { .BufferPool = &BufferPool };

	page_header Header = { 0 };
	Header.PageId = 0;
	Header.PageType = PageType_Directory;
	Header.ElementSize = sizeof(u32);
	Header.ElementSize2 = sizeof(u32);
	Header.ElementCount = 0;
	Header.DirectoryPageId = 0;
	Header.PreviousPageId = 0;
	Header.NextPageId = 0;

	page_slot * DirectoryPage = PageNew(&Context, Header);
	u32 DirectoryPageId = DirectoryPage->Header.PageId;

	bool8 Error = false;

	db_location Location;
	for (u32 I = 0; I < 2000000; I++)
	{
		Location = PageDirAppend(&Context, DirectoryPage, &I, sizeof(u32));
		DBContextUnpin(&Context, DirectoryPage);

		page_slot * LocationPage = PageLoad(&Context, Location.PageId);
		u32 * Value = PageGet(LocationPage, Location.Index).Data;
		DBContextUnpin(&Context, LocationPage);

		if (I != *Value)
		{
			Error = true;
		}
	}

	BufferPoolFlush(&BufferPool, false);

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