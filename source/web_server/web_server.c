#include "../reuse/base/base_include.h"
#include "../reuse/network/network_include.h"
#include "../reuse/io/io_include.h"

#include "../reuse/base/base_include.c"
#include "../reuse/network/network_include.c"
#include "../reuse/io/io_include.c"

#include "http_server.h"
#include "http_server.c"
#include "html.c"

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
			Request->ResponseBehavior = ResponseBehavior_Respond;
			Request->ResponseHTTPCode = 200;
			Request->ResponseBody = HTMLNodesTest(Server.ResponseArena);;
		}
	}
}