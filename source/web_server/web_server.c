#include "../reuse/base/base_include.h"
#include "../reuse/network/network_include.h"
#include "../reuse/io/io_include.h"

#include "../reuse/base/base_include.c"
#include "../reuse/network/network_include.c"
#include "../reuse/io/io_include.c"

#include "http_server.h"
#include "http_server.c"
#include "html.c"

str8 MainPage(memory_arena * Arena)
{
    html_writer Writer = HTMLWriterCreate(Arena);

    HTMLTag(&Writer, HTMLTag_head)
    {
        HTMLTag(&Writer, HTMLTag_title)
        {
            HTMLText(&Writer, Str8Lit("HTML Builder"));
        }
    }

    HTMLTag(&Writer, HTMLTag_body)
    {
        HTMLTag(&Writer, HTMLTag_p)
        {
            HTMLStyle(&Writer, HTMLStyle_color, Str8Lit("red"));
            HTMLText(&Writer, Str8Lit("Red"));
        }

        HTMLTag(&Writer, HTMLTag_p)
        {
            HTMLStyle(&Writer, HTMLStyle_color, Str8Lit("blue"));
            HTMLText(&Writer, Str8Lit("Blue"));
        }
    }

    return Str8FromHTML(Arena, Writer.DocumentRoot);
}

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
			Request->ResponseBody = MainPage(Server.ResponseArena);
		}
	}
}