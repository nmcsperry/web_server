#include "../../base/base_include.h"
#include "../../network/network_include.h"
#include "../../io/io_include.h"

#include "http_server.h"
#include "http_server.c"

void EntryHook()
{
	SocketInit();
	socket_handle Socket = SocketCreateServer(80, 50);
	http_server Server = ServerInit(Socket);

	while (true)
	{
		ServerLoop(&Server);

		http_request * Request = 0;
		while (Request = ServerNextRequest(&Server))
		{			
			str8 Body = Str8Fmt(Server.ResponseArena, "<html><body><p>Hello, this is the website.</p>"
				"<p>You are asking about %{str8}.</p>"
				"</body></html>", Request->Path);

			Request->ResponseBehavior = ResponseBehavior_Respond;
			Request->ResponseHTTPCode = 200;
			Request->ResponseBody = Body;
		}
	}
}