#include "../reuse/base/base_include.h"
#include "../reuse/network/network_include.h"
#include "../reuse/io/io_include.h"

#include "../reuse/base/base_include.c"
#include "../reuse/network/network_include.c"
#include "../reuse/io/io_include.c"

#include "server.h"
#include "server.c"
#include "html.c"

str8 NotFoundPage(web_server * Server, memory_arena * Arena)
{
    html_writer Writer = HTMLWriterCreate(Arena, 0, 0);

    HTMLTag(&Writer, HTMLTag_head)
    {
        HTMLTag(&Writer, HTMLTag_title)
        {
            HTMLText(&Writer, Str8Lit("404 Not Found"));
        }
    }

    HTMLTag(&Writer, HTMLTag_body)
    {
        HTMLSimpleTagCStr(&Writer, HTMLTag_p, "Requested Page Not Found");
    }

    return Str8FromHTML(Arena, Writer.DocumentRoot);
}

str8 WebSocketTestPage(memory_arena * Arena)
{
    html_writer Writer = HTMLWriterCreate(Arena, 0, 0);

    HTMLTag(&Writer, HTMLTag_head)
    {
        HTMLTag(&Writer, HTMLTag_title)
        {
            HTMLText(&Writer, Str8Lit("WebSocket Test"));
        }
    }

    HTMLTag(&Writer, HTMLTag_body)
    {
        HTMLTag(&Writer, HTMLTag_script)
        {
            HTMLAttr(&Writer, HTMLAttr_type, Str8Lit("text/javascript"));
            HTMLAttr(&Writer, HTMLAttr_src, Str8Lit("script.js"));
            HTMLTextCStr(&Writer, "var test;");
        }
    }

    return Str8FromHTML(Arena, Writer.DocumentRoot);
}

str8 MainPage(web_server * Server, memory_arena * Arena)
{
    html_writer Writer2 = HTMLWriterCreate(Arena, 0, 0);

    HTMLTag(&Writer2, HTMLTag_head)
    {
        HTMLTag(&Writer2, HTMLTag_title)
        {
            HTMLText(&Writer2, Str8Lit("HTML Builder"));
        }
    }

    HTMLTag(&Writer2, HTMLTag_body)
    {
        HTMLTag(&Writer2, HTMLTag_img)
        {
            HTMLAttr(&Writer2, HTMLAttr_src, Str8Lit("/my_image.png"));
        }

        HTMLTag(&Writer2, HTMLTag_p)
        {
            HTMLTag(&Writer2, HTMLTag_span)
            {
                HTMLStyle(&Writer2, HTMLStyle_color, Str8Lit("red"));
                HTMLText(&Writer2, Str8Lit("This is a red paragraph."));
            }
        }

        HTMLTagKey(&Writer2, HTMLTag_script, 7)
        {
            HTMLAttr(&Writer2, HTMLAttr_type, Str8Lit("text/javascript"));
            HTMLAttr(&Writer2, HTMLAttr_src, Str8Lit("script.js"));
            HTMLTextCStr(&Writer2, "var test;");
        }
    }

    return Str8FromHTML(Arena, Writer2.DocumentRoot);
}

str8 MainPageDiff(web_server * Server, memory_arena * Arena)
{
    html_writer Writer2 = HTMLWriterCreate(Arena, 0, 0);

    HTMLTag(&Writer2, HTMLTag_head)
    {
        HTMLTag(&Writer2, HTMLTag_title)
        {
            HTMLText(&Writer2, Str8Lit("HTML Builder"));
        }
    }

    HTMLTag(&Writer2, HTMLTag_body)
    {
        HTMLTag(&Writer2, HTMLTag_img)
        {
            HTMLAttr(&Writer2, HTMLAttr_src, Str8Lit("/my_image.png"));
        }

        HTMLTag(&Writer2, HTMLTag_p)
        {
            HTMLTag(&Writer2, HTMLTag_span)
            {
                HTMLStyle(&Writer2, HTMLStyle_color, Str8Lit("red"));
                HTMLText(&Writer2, Str8Lit("This is a red paragraph."));
            }
        }

        HTMLTagKey(&Writer2, HTMLTag_script, 7)
        {
            HTMLAttr(&Writer2, HTMLAttr_type, Str8Lit("text/javascript"));
            HTMLAttr(&Writer2, HTMLAttr_src, Str8Lit("script.js"));
            HTMLTextCStr(&Writer2, "var test;");
        }
    }

    html_writer Writer = HTMLWriterCreate(Arena, Writer2.DocumentRoot, Writer2.LastId);

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
            HTMLStyle(&Writer, HTMLStyle_color, Str8Lit("green"));
            HTMLText(&Writer, Str8Lit("This is a red paragraph."));
        }

        HTMLTagKey(&Writer, HTMLTag_script, 7)
        {
            HTMLAttr(&Writer, HTMLAttr_type, Str8Lit("text/javascript"));
            HTMLAttr(&Writer, HTMLAttr_src, Str8Lit("script.js"));
            HTMLTextCStr(&Writer, "var test;");
        }
    }

    StdOutputFmt("\r\n");
    for (html_diff * Diff = Writer.Diffs; Diff; Diff = Diff->Next)
    {
        StdOutputFmt("Old: %{str8}, New: %{str8}\r\n",
            Diff->Old ? Diff->Old->Type->Name : Str8Lit("NULL"),
            Diff->New ? Diff->New->Type->Name : Str8Lit("NULL"));
    }

    return Str8FromHTMLDiff(Arena, Writer.Diffs);
}

/*

for (u32 I = 0; I < ArrayCount(Server->Connections); I++)
{
    web_connection_slot * Connection = &Server->Connections[I];

    if (Connection->Value.Valid)
    {
        HTMLSimpleTagCStr(&Writer, HTMLTag_h3, "Connection");
        HTMLSimpleTagFmt(&Writer, HTMLTag_p, "IP Address: %{str8}", Str8FromIPAddr(Writer.Arena, Connection->Value.Address));

        datetime Datetime = DatetimeFromUnixTimeSec(Connection->Value.FirstCommunication);
        HTMLSimpleTagFmt(&Writer, HTMLTag_p, "First Communication: %{str8}", Str8FromDatetime(Writer.Arena, Datetime));

        temp_memory_arena Scratch = GetScratchArena(0, 0);
        ReleaseScratchArena(Scratch);

        HTMLSimpleTagFmt(&Writer, HTMLTag_p, "Total Requests: %{u32}", Connection->Value.RequestsReceived);
    }
}

*/

typedef struct asset
{
    str8 Data;
    mime_type * MimeType;
} asset;

void InitAssetPage(memory_arena * Arena, hash_table * HashTable, char * FilePath, str8 URL, mime_type * MimeType)
{
    str8 Extension = Str8CutFindFromEnd(URL, Str8Lit(".")).Second;

    asset * Asset = ArenaPush(Arena, asset);
    Asset->Data = FileInputFilename(FilePath, Arena);
    Asset->MimeType = MimeType;

    HashTableInsert(HashTable, URL, Asset);
}

hash_table * AssetHashTable;

void InitAssetPages(memory_arena * Arena)
{
    AssetHashTable = HashTableCreate(Arena, 64);

    InitAssetPage(Arena, AssetHashTable, "my_image.png", Str8Lit("/my_image.png"), &MimeType_PNG);
    InitAssetPage(Arena, AssetHashTable, "favicon.ico", Str8Lit("/favicon.ico"), &MimeType_ICO);
    InitAssetPage(Arena, AssetHashTable, "script.js", Str8Lit("/script.js"), &MimeType_JS);
}

void EntryHook()
{
	SocketInit();

    memory_arena * AssetArena = ArenaCreate(Megabytes(64));
    InitAssetPages(AssetArena);

	socket_handle Socket = SocketCreateServer(80, 50);
	web_server Server = ServerInit(Socket);

	while (true)
	{
		ServerLoop(&Server);

		web_request * Request = 0;
		while (Request = ServerNextRequest(&Server))
		{
            web_connection_slot * ConnectionSlot = &Server.Connections[Request->ConnectionIndex];

            if (ConnectionSlot->Value.ProtocolType == WebProtocol_WebSocket)
            {
                StdOutputFmt("Got a websocket request\n");
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseCode = 1;
                Request->ResponseBody = MainPageDiff(&Server, Server.ResponseArena);
                continue;
            }

            asset * Asset = HashTableGet(AssetHashTable, Request->RequestPath);
            if (Asset)
            {
                StdOutputFmt("Got an asset request\n\n");
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseCode = 200;
                Request->ResponseMimeType = Asset->MimeType;
                Request->ResponseBody = Asset->Data;
            }
            else if (Str8Match(Request->RequestPath, Str8Lit("/"), 0))
            {
                StdOutputFmt("Got root request\n\n");
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseCode = 200;
                Request->ResponseBody = MainPage(&Server, Server.ResponseArena);
                Request->ResponseMimeType = &MimeType_HTML;
            }
            else if (Str8Match(Request->RequestPath, Str8Lit("/post_test"), 0))
            {
                StdOutputFmt("Got a post request\n\n");
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseCode = 200;
                Request->ResponseBody = Str8Fmt(Server.ResponseArena, "%{str8} TEST", Request->RequestBody);
                Request->ResponseMimeType = &MimeType_HTML;
            }
            else
            {
                StdOutputFmt("Unknown request, responding with 404\n\n");
                Request->ResponseBehavior = ResponseBehavior_Respond;
                Request->ResponseCode = 404;
                Request->ResponseBody = NotFoundPage(&Server, Server.ResponseArena);
                Request->ResponseMimeType = &MimeType_HTML;
            }
		}
	}
}